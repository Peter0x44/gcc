------------------------------------------------------------------------------
--                                                                          --
--                         GNAT RUN-TIME COMPONENTS                         --
--                                                                          --
--                       S Y S T E M . P A C K _ 7 4                        --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 1992-2025, Free Software Foundation, Inc.         --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 3,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.                                     --
--                                                                          --
-- As a special exception under Section 7 of GPL version 3, you are granted --
-- additional permissions described in the GCC Runtime Library Exception,   --
-- version 3.1, as published by the Free Software Foundation.               --
--                                                                          --
-- You should have received a copy of the GNU General Public License and    --
-- a copy of the GCC Runtime Library Exception along with this program;     --
-- see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see    --
-- <http://www.gnu.org/licenses/>.                                          --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

--  Handling of packed arrays with Component_Size = 74

package System.Pack_74 is
   pragma Preelaborate;

   --  74-bit element type definition

   Bits : constant := 74;

   type Bits_74 is mod 2 ** Bits;
   for Bits_74'Size use Bits;

   --  In all subprograms below, Rev_SSO is set True if the array has the
   --  non-default scalar storage order.

   function Get_74
     (Arr     : System.Address;
      N       : Natural;
      Rev_SSO : Boolean) return Bits_74 with Inline;
   --  Arr is the address of the packed array, N is the zero-based
   --  subscript. This element is extracted and returned.

   procedure Set_74
     (Arr     : System.Address;
      N       : Natural;
      E       : Bits_74;
      Rev_SSO : Boolean) with Inline;
   --  Arr is the address of the packed array, N is the zero-based
   --  subscript. This element is set to the given value.

   function GetU_74
     (Arr     : System.Address;
      N       : Natural;
      Rev_SSO : Boolean) return Bits_74 with Inline;
   --  Arr is the address of the packed array, N is the zero-based
   --  subscript. This element is extracted and returned. This version
   --  is used when Arr may represent an unaligned address.

   procedure SetU_74
     (Arr     : System.Address;
      N       : Natural;
      E       : Bits_74;
      Rev_SSO : Boolean) with Inline;
   --  Arr is the address of the packed array, N is the zero-based
   --  subscript. This element is set to the given value. This version
   --  is used when Arr may represent an unaligned address

end System.Pack_74;
