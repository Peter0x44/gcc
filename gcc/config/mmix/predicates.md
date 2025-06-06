;; Operand and operator predicates for the GCC MMIX port.
;; Copyright (C) 2005-2025 Free Software Foundation, Inc.

;; This file is part of GCC.
;;
;; GCC is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 3, or (at your option)
;; any later version.
;;
;; GCC is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with GCC; see the file COPYING3.  If not see
;; <http://www.gnu.org/licenses/>.

;; Return 1 if OP is a valid comparison operator for "cbranch" instructions.
;; LE and GE are further lowered by the cbranchdf4 pattern.
(define_predicate "float_comparison_operator"
  (match_code "ne, eq, le, ge, lt, gt, ordered, unordered"))

;; True if this is a foldable comparison operator
;; - one where a the result of (compare:CC (reg) (const_int 0)) can be
;; replaced by (reg).  */

(define_predicate "mmix_foldable_comparison_operator"
  (match_code "ne, eq, ge, gt, le, lt, gtu, leu")
{
  RTX_CODE code = GET_CODE (op);

  if (mode == VOIDmode)
    mode = GET_MODE (op);

  /* This little bit is why the body of this predicate is kept as C.  */
  if (mode == VOIDmode)
    mode = GET_MODE (XEXP (op, 0));

  return ((mode == CCmode || mode == DImode)
	  && (code == NE || code == EQ || code == GE || code == GT
	      || code == LE || code == LT))
    /* FIXME: This may be a stupid trick.  What happens when GCC wants to
       reverse the condition?  Can it do that by itself?  Maybe it can
       even reverse the condition to fit a foldable one in the first
       place?  */
    || (mode == CC_UNSmode && (code == GTU || code == LEU));
})

;; Like comparison_operator, but only true if this comparison operator is
;; applied to a valid mode.  Needed to avoid jump.cc generating invalid
;; code with -ffast-math (gcc.dg/20001228-1.c).

(define_predicate "mmix_comparison_operator"
  (match_operand 0 "comparison_operator")
{
  RTX_CODE code = GET_CODE (op);

  /* Comparison operators usually don't have a mode, but let's try and get
     one anyway for the day that changes.  */
  if (mode == VOIDmode)
    mode = GET_MODE (op);

  /* Get the mode from the first operand if we don't have one.
     Also the reason why we do this in C.  */
  if (mode == VOIDmode)
    mode = GET_MODE (XEXP (op, 0));

  /* FIXME: This needs to be kept in sync with the tables in
     mmix_output_condition.  */
  return
    mode == VOIDmode
    || (mode == CC_FUNmode
	&& (code == ORDERED || code == UNORDERED))
    || (mode == CC_FPmode
	&& (code == GT || code == LT))
    || (mode == CC_FPEQmode
	&& (code == NE || code == EQ))
    || (mode == CC_UNSmode
	&& (code == GEU || code == GTU || code == LEU || code == LTU))
    || (mode == CCmode
	&& (code == NE || code == EQ || code == GE || code == GT
	    || code == LE || code == LT))
    || (mode == DImode
	&& (code == NE || code == EQ || code == GE || code == GT
	    || code == LE || code == LT || code == LEU || code == GTU));
})

;; True if this is a register with a condition-code mode.

(define_predicate "mmix_reg_cc_operand"
  (and (match_operand 0 "register_operand")
       (ior (match_test "GET_MODE (op) == CCmode")
	    (ior (match_test "GET_MODE (op) == CC_UNSmode")
		 (ior (match_test "GET_MODE (op) == CC_FPmode")
		      (ior (match_test "GET_MODE (op) == CC_FPEQmode")
			   (match_test "GET_MODE (op) == CC_FUNmode")))))))

;; True if this is an address_operand or a symbolic operand.

(define_predicate "mmix_symbolic_or_address_operand"
  (match_code "symbol_ref, label_ref, const, subreg, reg, plus")
{
  switch (GET_CODE (op))
    {
    case SYMBOL_REF:
    case LABEL_REF:
      return 1;
    case CONST:
      /* The reason why this body still is C.  */
      op = XEXP (op, 0);
      if ((GET_CODE (XEXP (op, 0)) == SYMBOL_REF
	   || GET_CODE (XEXP (op, 0)) == LABEL_REF)
	  && (GET_CODE (XEXP (op, 1)) == CONST_INT
	      || (GET_CODE (XEXP (op, 1)) == CONST_DOUBLE
		  && GET_MODE (XEXP (op, 1)) == VOIDmode)))
	return 1;
      /* Fall through.  */
    default:
      return mmix_address_operand (op, mode);
    }
})

;; True if this is a register or CONST_INT (or CONST_DOUBLE for DImode).
;; We could narrow the value down with a couple of predicates, but that
;; doesn't seem to be worth it at the moment.

(define_predicate "mmix_reg_or_constant_operand"
  (ior (match_operand 0 "register_operand")
       (ior (match_code "const_int")
	    (and (match_code "const_double")
		 (match_test "GET_MODE (op) == VOIDmode")))))

;; True if this is a register or 0 (int or float).

(define_predicate "mmix_reg_or_0_operand"
  (ior
   (match_operand 0 "register_operand")
   (ior
    (and (match_code "const_int")
	 (match_test "op == const0_rtx"))
    (and
     (match_code "const_double")
     ;; FIXME: Is mode calculation necessary and correct?
     (match_test
      "op == CONST0_RTX (mode == VOIDmode ? GET_MODE (op) : mode)")))))

;; True if this is a register or an int 0..255.

(define_predicate "mmix_reg_or_8bit_operand"
  (ior
   (match_operand 0 "register_operand")
   (and (match_code "const_int")
	(match_test "satisfies_constraint_I (op)"))))

;; True if this is a memory address, possibly strictly.
;; See also comment above the "*call_real" pattern.

(define_predicate "mmix_address_operand"
  (if_then_else (match_test "lra_in_progress || reload_completed")
    (match_test "strict_memory_address_p (Pmode, op)")
    (match_test "memory_address_p (Pmode, op)")))

(define_predicate "frame_pointer_operand"
  (ior
   (and
    (match_code "reg")
    (match_test "op == hard_frame_pointer_rtx || op == frame_pointer_rtx"))
   (and
    (match_code "plus")
    (match_code "reg" "0")
    (match_code "const_int" "1")
    (ior
     (match_test "XEXP (op, 0) == stack_pointer_rtx")
     ;; We can temporarily have a FP+offset here, where we (for FP)
     ;; accept only FP and the equivalent elimination of SP+offset.
     ;; See lra_eliminate_regs_1 in lra-eliminations.cc c:a line 370:
     ;;  "rtx to = subst_p ? ep->to_rtx : ep->from_rtx;"
     (and
      (match_test "lra_in_progress")
      (ior
       (match_test "XEXP (op, 0) == hard_frame_pointer_rtx")
       (match_test "XEXP (op, 0) == frame_pointer_rtx")))))))
