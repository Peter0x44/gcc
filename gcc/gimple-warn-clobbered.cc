/* -Wclobbered support.
   Copyright (C) 2026 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   GCC is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with GCC; see the file COPYING3.  If not see
   <http://www.gnu.org/licenses/>.  */

#include "config.h"
#include "system.h"
#include "coretypes.h"
#include "backend.h"
#include "tree.h"
#include "gimple.h"
#include "ssa.h"
#include "gimple-iterator.h"
#include "tree-cfg.h"
#include "fold-const.h"
#include "function.h"
#include "diagnostic-core.h"
#include "options.h"

/* After a longjmp, the values of the non-volatile automatic variables of
   the function that called setjmp are indeterminate if they were changed
   between the setjmp call and the longjmp (C11 7.13.2.1).  Warn about the
   variables that live in registers whose value might be used after setjmp
   returns for the second time and that might be changed after it returns
   for the first time.

   This is done just before expansion, while the CFG still has abnormal
   edges from the calls that might longjmp, through IFN_ABNORMAL_DISPATCHER
   blocks, to the blocks that start with returns-twice calls, and while the
   PHI nodes for those edges are still there.  RTL has no such edges, so RTL
   passes do not preserve that information.  */

/* What the -Wclobbered analysis needs to know about the current
   function.  */

struct setjmp_clobber_info
{
  setjmp_clobber_info ();
  ~setjmp_clobber_info ();

  /* The returns-twice calls.  */
  auto_vec<gcall *, 4> calls;

  /* For the call being analyzed, the statements after it in its basic
     block.  */
  hash_set<gimple *> after_this;

  /* For the call being analyzed, the basic blocks that might be executed
     after it returns, following the abnormal edges for the calls that
     might longjmp too, the ones among them whose last statement might be
     executed, and the statements in them that cannot be executed while
     its context is still valid.  */
  sbitmap reachable;
  sbitmap end_reachable;
  hash_set<gimple *> dead;

  /* For the call being analyzed, the basic blocks that might be executed
     after it returns for the second time and before it is called again,
     not counting the one containing the call.  */
  sbitmap region;

  /* For the call being analyzed, the SSA names whose value is known to be
     nonzero after a second return if the second element is true, and zero
     if it is false.  */
  auto_vec<std::pair<tree, bool>, 4> derived;

  /* For the call being analyzed, whether the value of an SSA name might be
     used from a basic block, see setjmp_name_used_from_p, the queries of
     that kind being answered, and how many more queries can be answered
     before giving up.  */
  typedef pair_hash<nofree_ptr_hash<tree_node>,
		    nofree_ptr_hash<basic_block_def>> name_block_hash;
  hash_map<name_block_hash, bool> used;
  hash_set<name_block_hash> used_in_progress;
  unsigned int used_budget;
  unsigned int used_depth;

  /* Scratch space for liveness.  */
  sbitmap live;
  auto_vec<basic_block, 16> worklist;
};

setjmp_clobber_info::setjmp_clobber_info ()
{
  reachable = sbitmap_alloc (last_basic_block_for_fn (cfun));
  end_reachable = sbitmap_alloc (last_basic_block_for_fn (cfun));
  region = sbitmap_alloc (last_basic_block_for_fn (cfun));
  live = sbitmap_alloc (last_basic_block_for_fn (cfun));
}

setjmp_clobber_info::~setjmp_clobber_info ()
{
  sbitmap_free (reachable);
  sbitmap_free (end_reachable);
  sbitmap_free (region);
  sbitmap_free (live);
}

/* Return true if STMT might be executed after the returns-twice call
   being analyzed returns.  */

static bool
setjmp_reexecuted_p (setjmp_clobber_info &info, gimple *stmt)
{
  return ((bitmap_bit_p (info.reachable, gimple_bb (stmt)->index)
	   || info.after_this.contains (stmt))
	  && !info.dead.contains (stmt));
}

/* Return the name of the function called by the returns-twice call STMT
   without a _ or __ prefix if it might be one of the functions recognized
   by special_function_p, or NULL otherwise.  */

static const char *
setjmp_special_name (const gcall *stmt)
{
  tree fndecl = gimple_call_fndecl (stmt);
  if (!fndecl || !DECL_NAME (fndecl) || !maybe_special_function_p (fndecl))
    return NULL;
  const char *name = IDENTIFIER_POINTER (DECL_NAME (fndecl));
  if (name[0] == '_')
    name += name[1] == '_' ? 2 : 1;
  return name;
}

/* Return true if the returns-twice call STMT is known to be to one of the
   standard functions that return twice, none of which longjmp.  */

static bool
setjmp_known_function_p (const gcall *stmt)
{
  const char *name = setjmp_special_name (stmt);
  return (name
	  && (!strcmp (name, "setjmp")
	      || !strcmp (name, "sigsetjmp")
	      || !strcmp (name, "vfork")
	      || !strcmp (name, "getcontext")
	      || !strcmp (name, "savectx")));
}

/* Return true if the returns-twice call STMT is known to return a nonzero
   value when it returns for the second time, which is the case for setjmp
   and sigsetjmp (when reached through longjmp) and for vfork (when the
   parent resumes), but not e.g. for getcontext.  */

static bool
setjmp_second_return_nonzero_p (const gcall *stmt)
{
  const char *name = setjmp_special_name (stmt);
  return (name
	  && (!strcmp (name, "setjmp")
	      || !strcmp (name, "sigsetjmp")
	      || !strcmp (name, "vfork")));
}

/* Return true if executing the returns-twice call STMT again means that it
   can no longer return for the second time from an earlier execution:
   vfork only resumes the parent after the latest child, and a longjmp
   restores the context saved by the latest setjmp with the same buffer,
   which is always the case if the buffer is invariant.  */

static bool
setjmp_replaces_context_p (const gcall *stmt)
{
  const char *name = setjmp_special_name (stmt);
  if (!name)
    return false;
  if (!strcmp (name, "vfork"))
    return true;
  return ((!strcmp (name, "setjmp")
	   || !strcmp (name, "sigsetjmp")
	   || !strcmp (name, "getcontext")
	   || !strcmp (name, "savectx"))
	  && gimple_call_num_args (stmt) > 0
	  && is_gimple_min_invariant (gimple_call_arg (stmt, 0)));
}

/* Set INFO.derived for the returns-twice call CALL.  */

static void
setjmp_compute_derived (setjmp_clobber_info &info, gcall *call)
{
  info.derived.truncate (0);
  tree res = gimple_call_lhs (call);
  if (!res
      || TREE_CODE (res) != SSA_NAME
      || !INTEGRAL_TYPE_P (TREE_TYPE (res))
      || !setjmp_second_return_nonzero_p (call))
    return;

  info.derived.safe_push (std::make_pair (res, true));
  for (unsigned int i = 0; i < info.derived.length (); i++)
    {
      tree name = info.derived[i].first;
      bool nonzero = info.derived[i].second;
      imm_use_iterator iter;
      gimple *use_stmt;
      FOR_EACH_IMM_USE_STMT (use_stmt, iter, name)
	{
	  gassign *assign = dyn_cast <gassign *> (use_stmt);
	  if (!assign || gimple_assign_rhs1 (assign) != name)
	    continue;
	  tree lhs = gimple_assign_lhs (assign);
	  if (TREE_CODE (lhs) != SSA_NAME || !INTEGRAL_TYPE_P (TREE_TYPE (lhs)))
	    continue;

	  /* Copies and conversions that cannot drop nonzero bits keep the
	     value nonzero, and comparisons against zero turn it into a
	     known truth value.  */
	  enum tree_code code = gimple_assign_rhs_code (assign);
	  bool lhs_nonzero;
	  if (code == SSA_NAME || CONVERT_EXPR_CODE_P (code))
	    {
	      if (TYPE_PRECISION (TREE_TYPE (lhs))
		  < TYPE_PRECISION (TREE_TYPE (name)))
		continue;
	      lhs_nonzero = nonzero;
	    }
	  else if ((code == NE_EXPR || code == EQ_EXPR)
		   && integer_zerop (gimple_assign_rhs2 (assign)))
	    lhs_nonzero = nonzero == (code == NE_EXPR);
	  else
	    continue;

	  bool seen = false;
	  for (auto &d : info.derived)
	    if (d.first == lhs)
	      seen = true;
	  if (!seen)
	    info.derived.safe_push (std::make_pair (lhs, lhs_nonzero));
	}
    }
}

/* Return true if E might be taken after a second return of the
   returns-twice call being analyzed, given INFO.derived, if its source is
   executed then.  */

static bool
setjmp_edge_possible_p (setjmp_clobber_info &info, edge e)
{
  if (e->flags & EDGE_ABNORMAL)
    return false;

  gimple *last = *gsi_last_bb (e->src);
  if (!last)
    return true;

  if (gcond *cond = dyn_cast <gcond *> (last))
    {
      enum tree_code code = gimple_cond_code (cond);
      if ((code != EQ_EXPR && code != NE_EXPR)
	  || !integer_zerop (gimple_cond_rhs (cond)))
	return true;
      for (auto &d : info.derived)
	if (d.first == gimple_cond_lhs (cond))
	  {
	    bool cond_true = d.second == (code == NE_EXPR);
	    return (e->flags & (cond_true ? EDGE_TRUE_VALUE
				: EDGE_FALSE_VALUE)) != 0;
	  }
      return true;
    }

  if (gswitch *sw = dyn_cast <gswitch *> (last))
    {
      bool nonzero = false, known = false;
      for (auto &d : info.derived)
	if (d.first == gimple_switch_index (sw))
	  {
	    nonzero = d.second;
	    known = true;
	  }
      if (!known)
	return true;

      /* Look for a case label that leads to E and that is possible for the
	 value of the index, or for the default label when no case label
	 handles it.  */
      bool zero_handled = false;
      for (unsigned int i = 1; i < gimple_switch_num_labels (sw); i++)
	{
	  tree label = gimple_switch_label (sw, i);
	  tree low = CASE_LOW (label);
	  tree high = CASE_HIGH (label) ? CASE_HIGH (label) : low;
	  bool has_zero = (tree_int_cst_sgn (low) <= 0
			   && tree_int_cst_sgn (high) >= 0);
	  bool has_nonzero = !integer_zerop (low) || !integer_zerop (high);
	  zero_handled |= has_zero;
	  if ((nonzero ? has_nonzero : has_zero)
	      && label_to_block (cfun, CASE_LABEL (label)) == e->dest)
	    return true;
	}
      tree label = gimple_switch_default_label (sw);
      return ((nonzero || !zero_handled)
	      && label_to_block (cfun, CASE_LABEL (label)) == e->dest);
    }

  return true;
}

/* Return true if E might be taken after a second return of the
   returns-twice call CALL being analyzed and before CALL is executed
   again.  */

static bool
setjmp_region_edge_p (setjmp_clobber_info &info, gcall *call, edge e)
{
  return (e->dest != gimple_bb (call)
	  && e->dest != EXIT_BLOCK_PTR_FOR_FN (cfun)
	  && (e->src == gimple_bb (call)
	      || bitmap_bit_p (info.region, e->src->index))
	  && setjmp_edge_possible_p (info, e));
}

/* Return true if E is an abnormal edge for a call that cannot longjmp
   because it is a call to a standard function that returns twice.  */

static bool
setjmp_spurious_abnormal_edge_p (edge e)
{
  if (!(e->flags & EDGE_ABNORMAL))
    return false;
  gcall *last = safe_dyn_cast <gcall *> (*gsi_last_bb (e->src));
  return (last
	  && (gimple_call_flags (last) & ECF_RETURNS_TWICE)
	  && setjmp_known_function_p (last));
}

/* Return the local variable whose address the returns-twice call CALL
   saves its context in, if it is known to, or NULL otherwise.  */

static tree
setjmp_context_var (const gcall *call)
{
  if (!setjmp_replaces_context_p (call) || gimple_call_num_args (call) == 0)
    return NULL_TREE;
  tree arg = gimple_call_arg (call, 0);
  if (TREE_CODE (arg) != ADDR_EXPR)
    return NULL_TREE;
  tree base = get_base_address (TREE_OPERAND (arg, 0));
  if (!base || !VAR_P (base) || is_global_var (base))
    return NULL_TREE;
  return base;
}

/* Mark the statements from GSI on as dead in INFO if one of them ends the
   storage of CONTEXT, and return whether that is the case.  */

static bool
setjmp_mark_dead (setjmp_clobber_info &info, gimple_stmt_iterator gsi,
		  tree context)
{
  if (!context)
    return false;
  bool dead = false;
  for (; !gsi_end_p (gsi); gsi_next (&gsi))
    {
      gimple *stmt = gsi_stmt (gsi);
      if (!dead
	  && gimple_clobber_p (stmt, CLOBBER_STORAGE_END)
	  && get_base_address (gimple_assign_lhs (stmt)) == context)
	dead = true;
      if (dead)
	info.dead.add (stmt);
    }
  return dead;
}

/* Set INFO.reachable, INFO.end_reachable and INFO.dead for the
   returns-twice call CALL.  */

static void
setjmp_compute_reachable (setjmp_clobber_info &info, gcall *call)
{
  /* If executing CALL again replaces the context that it saved, what is
     executed after that does not matter, and neither does what is executed
     after the storage for the context ends.  */
  basic_block call_bb = gimple_bb (call);
  bool stop = setjmp_replaces_context_p (call);
  tree context = setjmp_context_var (call);
  bitmap_clear (info.reachable);
  bitmap_clear (info.end_reachable);
  info.dead.empty ();
  info.worklist.truncate (0);

  gimple_stmt_iterator gsi = gsi_for_stmt (call);
  gsi_next (&gsi);
  if (!setjmp_mark_dead (info, gsi, context))
    info.worklist.safe_push (call_bb);
  while (!info.worklist.is_empty ())
    {
      basic_block bb = info.worklist.pop ();
      bitmap_set_bit (info.end_reachable, bb->index);
      edge e;
      edge_iterator ei;
      FOR_EACH_EDGE (e, ei, bb->succs)
	if (!setjmp_spurious_abnormal_edge_p (e)
	    && !(stop && e->dest == call_bb)
	    && bitmap_set_bit (info.reachable, e->dest->index)
	    && (e->dest == EXIT_BLOCK_PTR_FOR_FN (cfun)
		|| !setjmp_mark_dead (info, gsi_start_bb (e->dest), context)))
	  info.worklist.safe_push (e->dest);
    }
}

/* Set INFO.region for the returns-twice call CALL.  */

static void
setjmp_compute_region (setjmp_clobber_info &info, gcall *call)
{
  bitmap_clear (info.region);
  info.worklist.truncate (0);
  info.worklist.safe_push (gimple_bb (call));
  while (!info.worklist.is_empty ())
    {
      basic_block bb = info.worklist.pop ();
      edge e;
      edge_iterator ei;
      FOR_EACH_EDGE (e, ei, bb->succs)
	if (setjmp_region_edge_p (info, call, e)
	    && bitmap_set_bit (info.region, e->dest->index))
	  info.worklist.safe_push (e->dest);
    }
}

/* Return true if the value of the SSA name NAME might be used in the
   region of the returns-twice call CALL being analyzed when it reaches
   START, which is either the point after CALL if START is its block, or
   the point where NAME is defined if START is its block, or the point
   after the PHI nodes of START.  An argument of a PHI node is only used
   if the result of the PHI node is, and similarly for an operand of a
   statement that just computes a value.

   A query that is already being answered is assumed to be false, which
   is what it is when the uses form a cycle that nothing else uses.  Set
   *PROVISIONAL if the result depends on that assumption, in which case it
   is not recorded unless it is true.  */

static bool
setjmp_name_used_from_p (setjmp_clobber_info &info, gcall *call, tree name,
			 basic_block start, bool *provisional)
{
  auto key = std::make_pair (name, start);
  if (bool *cached = info.used.get (key))
    return *cached;
  if (info.used_in_progress.contains (key))
    {
      *provisional = true;
      return false;
    }
  if (info.used_budget == 0 || info.used_depth >= 1000)
    return true;
  info.used_budget--;
  info.used_depth++;
  info.used_in_progress.add (key);
  bool depends = false;

  basic_block def_bb = gimple_bb (SSA_NAME_DEF_STMT (name));
  basic_block call_bb = gimple_bb (call);
  auto_bitmap live;
  auto_vec<basic_block, 16> worklist;
  bool used = false;

  imm_use_iterator iter;
  use_operand_p use_p;
  FOR_EACH_IMM_USE_FAST (use_p, iter, name)
    {
      gimple *use_stmt = USE_STMT (use_p);
      if (is_gimple_debug (use_stmt))
	continue;
      basic_block bb;
      if (gphi *phi = dyn_cast <gphi *> (use_stmt))
	{
	  edge e = gimple_phi_arg_edge (phi, PHI_ARG_INDEX_FROM_USE (use_p));
	  if (!setjmp_region_edge_p (info, call, e)
	      || !setjmp_name_used_from_p (info, call, gimple_phi_result (phi),
					   e->dest, &depends))
	    continue;
	  bb = e->src;
	  if (bb == start)
	    {
	      used = true;
	      break;
	    }
	}
      else
	{
	  bb = gimple_bb (use_stmt);
	  if (bb == start)
	    {
	      if (start == call_bb && !info.after_this.contains (use_stmt))
		continue;
	    }
	  else if (!bitmap_bit_p (info.region, bb->index))
	    continue;

	  /* Look through statements that just compute a value.  */
	  gassign *assign = dyn_cast <gassign *> (use_stmt);
	  if (assign
	      && TREE_CODE (gimple_assign_lhs (assign)) == SSA_NAME
	      && !gimple_vdef (assign)
	      && !gimple_has_side_effects (assign)
	      && !gimple_could_trap_p (assign)
	      && !setjmp_name_used_from_p (info, call,
					   gimple_assign_lhs (assign), bb,
					   &depends))
	    continue;

	  if (bb == start)
	    {
	      used = true;
	      break;
	    }
	}
      if (bb != def_bb && bitmap_set_bit (live, bb->index))
	worklist.safe_push (bb);
    }

  while (!used && !worklist.is_empty ())
    {
      basic_block bb = worklist.pop ();
      edge e;
      edge_iterator ei;
      FOR_EACH_EDGE (e, ei, bb->preds)
	if (setjmp_region_edge_p (info, call, e))
	  {
	    if (e->src == start)
	      {
		used = true;
		break;
	      }
	    if (e->src != def_bb && bitmap_set_bit (live, e->src->index))
	      worklist.safe_push (e->src);
	  }
    }

  info.used_in_progress.remove (key);
  info.used_depth--;
  if (used || !depends)
    info.used.put (key, used);
  else
    *provisional = true;
  return used;
}

/* Return true if the SSA name NAME might be used after a second return of
   the returns-twice call CALL, i.e. if it is live after CALL, considering
   only what might be executed then.  */

static bool
setjmp_name_used_p (setjmp_clobber_info &info, gcall *call, tree name)
{
  /* A value computed after CALL is not the value at CALL.  */
  gimple *def = SSA_NAME_DEF_STMT (name);
  if (def == call || info.after_this.contains (def))
    return false;
  bool provisional = false;
  return setjmp_name_used_from_p (info, call, name, gimple_bb (call),
				  &provisional);
}

/* Return NAME, or what it is a copy of.  */

static tree
setjmp_strip_copies (tree name)
{
  for (unsigned int i = 0; i < 64 && TREE_CODE (name) == SSA_NAME; i++)
    {
      gimple *def = SSA_NAME_DEF_STMT (name);
      if (!gimple_assign_ssa_name_copy_p (def))
	break;
      name = gimple_assign_rhs1 (def);
    }
  return name;
}

/* Return true if a block that is executed after the returns-twice call
   CALL being analyzed returns might give the PHI node PHI a value other
   than its result, either directly or, for a receiver, through a
   dispatcher.  */

static bool
setjmp_phi_changed_p (setjmp_clobber_info &info, gcall *call, gphi *phi)
{
  tree res = gimple_phi_result (phi);
  tree value = setjmp_strip_copies (res);
  /* When CALL is executed again in a way that replaces its context, its
     receiver gets a new value for a new context.  */
  bool new_context = (gimple_bb (phi) == gimple_bb (call)
		      && setjmp_replaces_context_p (call));
  edge e;
  edge_iterator ei;
  FOR_EACH_EDGE (e, ei, gimple_bb (phi)->preds)
    {
      tree arg = PHI_ARG_DEF_FROM_EDGE (phi, e);
      if (setjmp_strip_copies (arg) == value)
	continue;

      gimple_stmt_iterator gsi = gsi_start_nondebug_after_labels_bb (e->src);
      if ((e->flags & EDGE_ABNORMAL)
	  && !gsi_end_p (gsi)
	  && gimple_call_internal_p (gsi_stmt (gsi), IFN_ABNORMAL_DISPATCHER))
	{
	  /* The value at the calls that might longjmp, which only matters
	     for the ones executed after a returns-twice call returns.  */
	  gphi *dphi = (TREE_CODE (arg) == SSA_NAME
			? dyn_cast <gphi *> (SSA_NAME_DEF_STMT (arg)) : NULL);
	  edge de;
	  edge_iterator dei;
	  FOR_EACH_EDGE (de, dei, e->src->preds)
	    if (!setjmp_spurious_abnormal_edge_p (de)
		&& bitmap_bit_p (info.end_reachable, de->src->index)
		&& (!dphi
		    || gimple_bb (dphi) != e->src
		    || (setjmp_strip_copies (PHI_ARG_DEF_FROM_EDGE (dphi, de))
			!= value)))
	      return true;
	}
      else if (!new_context
	       && bitmap_bit_p (info.end_reachable, e->src->index))
	return true;
    }
  return false;
}

/* Return the value of the SSA name NAME if the copies and PHI nodes that
   define it only ever give it one value, or NAME otherwise.  */

static tree
setjmp_single_value (tree name)
{
  auto_vec<tree, 16> worklist;
  hash_set<tree> visited;
  tree value = NULL_TREE;
  worklist.safe_push (name);
  visited.add (name);
  while (!worklist.is_empty ())
    {
      tree t = worklist.pop ();
      gimple *def = (TREE_CODE (t) == SSA_NAME && !SSA_NAME_IS_DEFAULT_DEF (t)
		     ? SSA_NAME_DEF_STMT (t) : NULL);
      if (def && gimple_code (def) == GIMPLE_PHI)
	{
	  for (unsigned int i = 0; i < gimple_phi_num_args (def); i++)
	    {
	      tree arg = gimple_phi_arg_def (def, i);
	      if (!visited.add (arg))
		worklist.safe_push (arg);
	    }
	}
      else if (def && gimple_assign_ssa_name_copy_p (def))
	{
	  tree src = gimple_assign_rhs1 (def);
	  if (!visited.add (src))
	    worklist.safe_push (src);
	}
      else if (!value)
	value = t;
      else if (!operand_equal_p (value, t))
	return name;
      if (visited.elements () > 64)
	return name;
    }
  return value ? value : name;
}

/* Return true if the value of the SSA name NAME might change after the
   returns-twice call CALL being analyzed returns, i.e. if its definition
   might be executed again then with a different result.  DEPTH is the
   depth of recursion.  */

static bool
setjmp_name_changed_p (setjmp_clobber_info &info, gcall *call, tree name,
		       unsigned int depth = 0)
{
  if (SSA_NAME_IS_DEFAULT_DEF (name))
    return false;
  gimple *def = SSA_NAME_DEF_STMT (name);
  tree value = setjmp_single_value (name);
  if (value != name)
    return (TREE_CODE (value) == SSA_NAME
	    && (depth > 10
		|| setjmp_name_changed_p (info, call, value, depth + 1)));
  if (gphi *phi = dyn_cast <gphi *> (def))
    return setjmp_phi_changed_p (info, call, phi);
  return setjmp_reexecuted_p (info, def);
}

/* Set *READS and *WRITES to whether STMT reads and writes the hard
   register variable VAR.  */

static void
setjmp_hard_reg_accesses (gimple *stmt, tree var, bool *reads, bool *writes)
{
  *reads = *writes = false;
  auto refers_to = [var] (tree op)
    {
      return op && walk_tree_without_duplicates
		     (&op, [] (tree *tp, int *, void *data) -> tree
			{ return *tp == (tree) data ? *tp : NULL_TREE; },
		      var) != NULL_TREE;
    };

  if (gasm *asm_stmt = dyn_cast <gasm *> (stmt))
    {
      for (unsigned int i = 0; i < gimple_asm_noutputs (asm_stmt); i++)
	if (refers_to (TREE_VALUE (gimple_asm_output_op (asm_stmt, i))))
	  *writes = true;
      for (unsigned int i = 0; i < gimple_asm_ninputs (asm_stmt); i++)
	if (refers_to (TREE_VALUE (gimple_asm_input_op (asm_stmt, i))))
	  *reads = true;
      return;
    }

  tree lhs = gimple_get_lhs (stmt);
  for (unsigned int i = 0; i < gimple_num_ops (stmt); i++)
    {
      tree op = gimple_op (stmt, i);
      if (i == 0 && lhs)
	{
	  /* A partial store also reads the rest of VAR.  */
	  if (lhs == var)
	    *writes = true;
	  else if (refers_to (lhs))
	    *reads = *writes = true;
	}
      else if (refers_to (op))
	*reads = true;
    }
}

/* Return true if the hard register variable VAR might be used after a
   second return of the returns-twice call CALL.  */

static bool
setjmp_hard_reg_used_p (setjmp_clobber_info &info, gcall *call, tree var)
{
  bool reads, writes;
  basic_block call_bb = gimple_bb (call);

  /* First the statements after CALL in its block.  */
  gimple_stmt_iterator gsi = gsi_for_stmt (call);
  for (gsi_next (&gsi); !gsi_end_p (gsi); gsi_next (&gsi))
    {
      if (is_gimple_debug (gsi_stmt (gsi)))
	continue;
      setjmp_hard_reg_accesses (gsi_stmt (gsi), var, &reads, &writes);
      if (reads)
	return true;
      if (writes)
	return false;
    }

  /* Then the blocks in the region, by computing the blocks where VAR is
     live on entry, or where it is read before being written (GEN) and
     where it is written (KILL).  */
  auto_sbitmap gen (last_basic_block_for_fn (cfun));
  auto_sbitmap kill (last_basic_block_for_fn (cfun));
  bitmap_clear (gen);
  bitmap_clear (kill);
  bitmap_clear (info.live);
  info.worklist.truncate (0);
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    {
      if (!bitmap_bit_p (info.region, bb->index))
	continue;
      for (gsi = gsi_start_bb (bb); !gsi_end_p (gsi); gsi_next (&gsi))
	{
	  if (is_gimple_debug (gsi_stmt (gsi)))
	    continue;
	  setjmp_hard_reg_accesses (gsi_stmt (gsi), var, &reads, &writes);
	  if (reads)
	    {
	      bitmap_set_bit (gen, bb->index);
	      break;
	    }
	  if (writes)
	    {
	      bitmap_set_bit (kill, bb->index);
	      break;
	    }
	}
      if (bitmap_bit_p (gen, bb->index))
	{
	  bitmap_set_bit (info.live, bb->index);
	  info.worklist.safe_push (bb);
	}
    }

  while (!info.worklist.is_empty ())
    {
      bb = info.worklist.pop ();
      edge e;
      edge_iterator ei;
      FOR_EACH_EDGE (e, ei, bb->preds)
	if (setjmp_region_edge_p (info, call, e))
	  {
	    if (e->src == call_bb)
	      return true;
	    if (!bitmap_bit_p (kill, e->src->index)
		&& bitmap_set_bit (info.live, e->src->index))
	      info.worklist.safe_push (e->src);
	  }
    }
  return false;
}

/* Return true if the hard register variable VAR might change after a
   returns-twice call returns.  */

static bool
setjmp_hard_reg_changed_p (setjmp_clobber_info &info, tree var)
{
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
	 gsi_next (&gsi))
      {
	gimple *stmt = gsi_stmt (gsi);
	bool reads, writes;
	if (is_gimple_debug (stmt) || !setjmp_reexecuted_p (info, stmt))
	  continue;
	setjmp_hard_reg_accesses (stmt, var, &reads, &writes);
	if (writes)
	  return true;
      }
  return false;
}

/* Record in DECL_BLOCKS the scope block of each variable declared in BLOCK
   or its subblocks.  */

static void
setjmp_record_decl_blocks (hash_map<tree, tree> &decl_blocks, tree block)
{
  for (tree decl = BLOCK_VARS (block); decl; decl = DECL_CHAIN (decl))
    if (VAR_P (decl))
      decl_blocks.put (decl, block);
  for (tree sub = BLOCK_SUBBLOCKS (block); sub; sub = BLOCK_CHAIN (sub))
    setjmp_record_decl_blocks (decl_blocks, sub);
}

/* Return true if the variable VAR might be in scope at the returns-twice
   call CALL, given the scope blocks DECL_BLOCKS.  A variable whose scope
   starts after CALL is a new object each time, so it cannot be clobbered.  */

static bool
setjmp_in_scope_p (hash_map<tree, tree> &decl_blocks, gcall *call, tree var)
{
  tree *var_block = decl_blocks.get (var);
  tree block = gimple_block (call);
  if (!var_block || !block)
    return true;
  for (; block && TREE_CODE (block) == BLOCK;
       block = BLOCK_SUPERCONTEXT (block))
    if (block == *var_block)
      return true;
  return false;
}

/* Warn about the variables and arguments of the current function that
   might be clobbered by longjmp or vfork.  */

void
generate_setjmp_warnings (void)
{
  if (!warn_clobbered || !cfun->calls_setjmp)
    return;

  setjmp_clobber_info info;
  basic_block bb;
  FOR_EACH_BB_FN (bb, cfun)
    for (gimple_stmt_iterator gsi = gsi_start_bb (bb); !gsi_end_p (gsi);
	 gsi_next (&gsi))
      {
	gimple *stmt = gsi_stmt (gsi);
	/* The transactional memory runtime restores what it needs to when
	   it restarts a transaction.  */
	if (is_gimple_call (stmt)
	    && (gimple_call_flags (stmt) & ECF_RETURNS_TWICE)
	    && !gimple_call_builtin_p (stmt, BUILT_IN_TM_START))
	  info.calls.safe_push (as_a <gcall *> (stmt));
      }

  /* The register variables that might be clobbered, i.e. that are not
     volatile or addressable, as for use_register_for_decl, and the hard
     register variables.  */
  auto_vec<tree> ssa_names;
  unsigned int i;
  tree name;
  FOR_EACH_SSA_NAME (i, name, cfun)
    {
      tree var = SSA_NAME_VAR (name);
      if (var
	  && (VAR_P (var) || TREE_CODE (var) == PARM_DECL)
	  && !DECL_ARTIFICIAL (var)
	  && !virtual_operand_p (name)
	  && use_register_for_decl (name))
	ssa_names.safe_push (name);
    }
  auto_vec<tree> hard_regs;
  tree var;
  FOR_EACH_LOCAL_DECL (cfun, i, var)
    if (VAR_P (var) && DECL_HARD_REGISTER (var) && !is_global_var (var))
      hard_regs.safe_push (var);

  hash_map<tree, tree> decl_blocks;
  setjmp_record_decl_blocks (decl_blocks, DECL_INITIAL (current_function_decl));

  auto_bitmap clobbered;
  auto_vec<tree> to_warn;
  for (gcall *call : info.calls)
    {
      info.after_this.empty ();
      info.used.empty ();
      info.used_budget = 100000;
      info.used_depth = 0;
      gimple_stmt_iterator gsi = gsi_for_stmt (call);
      for (gsi_next (&gsi); !gsi_end_p (gsi); gsi_next (&gsi))
	info.after_this.add (gsi_stmt (gsi));
      setjmp_compute_reachable (info, call);
      setjmp_compute_derived (info, call);
      setjmp_compute_region (info, call);
      for (tree name : ssa_names)
	{
	  var = SSA_NAME_VAR (name);
	  if (!bitmap_bit_p (clobbered, DECL_UID (var))
	      && setjmp_in_scope_p (decl_blocks, call, var)
	      && setjmp_name_used_p (info, call, name)
	      && setjmp_name_changed_p (info, call, name)
	      && bitmap_set_bit (clobbered, DECL_UID (var)))
	    to_warn.safe_push (var);
	}
      for (tree hard_reg : hard_regs)
	if (!bitmap_bit_p (clobbered, DECL_UID (hard_reg))
	    && setjmp_in_scope_p (decl_blocks, call, hard_reg)
	    && setjmp_hard_reg_used_p (info, call, hard_reg)
	    && setjmp_hard_reg_changed_p (info, hard_reg)
	    && bitmap_set_bit (clobbered, DECL_UID (hard_reg)))
	  to_warn.safe_push (hard_reg);
    }

  to_warn.qsort ([] (const void *a, const void *b)
		 {
		   return ((int) DECL_UID (*(const tree *) a)
			   - (int) DECL_UID (*(const tree *) b));
		 });
  for (tree decl : to_warn)
    if (TREE_CODE (decl) == PARM_DECL)
      warning (OPT_Wclobbered,
	       "argument %q+D might be clobbered by %<longjmp%> or %<vfork%>",
	       decl);
    else
      warning (OPT_Wclobbered,
	       "variable %q+D might be clobbered by %<longjmp%> or %<vfork%>",
	       decl);
}
