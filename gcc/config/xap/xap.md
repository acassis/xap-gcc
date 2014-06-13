
(define_constants [(RAH 0) (RAL 1) (RXH 2) (RXL 3)
		   (RAP 4) (RSP 5) (RFP 6) (RSPECIAL 7)
		   (RMAXIM 8) (RMAJOR 9) (RMINOR 10) (RMINIM 11)
		   (RBC 12) (RTMP 13) (RSAVED_Y 14) (RSCRATCH 15)
	           (FIRST_PSEUDO_REGISTER 24) (BITS_PER_UNIT 16)])

;; length default is 1 word each
(define_attr "length" "" (const_int 1))

(define_expand "canonicalize_funcptr_for_compare"
  [(set (match_operand 0 "nonimmediate_operand" "")
        (match_operand 1 "general_operand" ""))
  ]
  ""
  "
{
if (XAP_FLAG_ENABLED(BIG_FUNCTION_PTR))
    {
    if (xap2plus_expand_movhi(operands)) DONE;
    }
else
    {
    if (xap2plus_expand_movqi(operands)) DONE;
    }
}")

;;;; From the GCC 2.95.2 documentation, insns definitions are interleaved
;;
;;;; `movm' 
;;;;     Here m stands for a two-letter machine mode name, in lower case.
;;;;     This instruction pattern moves data with that machine mode from
;;;;     operand 1 to operand 0. For example, `movsi' moves full-word
;;;;     data. If operand 0 is a subreg with mode m of a register whose own
;;;;     mode is wider than m, the effect of this instruction is to store the
;;;;     specified value in the part of the register that corresponds to mode
;;;;     m. The effect on the rest of the register is undefined. This class of
;;;;     patterns is special in several ways. First of all, each of these names
;;;;     up to and including full word size must be defined, because there is
;;;;     no other way to copy a datum from one place to another. If there are
;;;;     patterns accepting operands in larger modes, `movm' must be
;;;;     defined for integer modes of those sizes. Second, these patterns are
;;;;     not used solely in the RTL generation pass. Even the reload pass can
;;;;     generate move insns to copy values from stack slots into temporary
;;;;     registers. When it does so, one of the operands is a hard register and
;;;;     the other is an operand that can need to be reloaded into a register.
;;;;     Therefore, when given such a pair of operands, the pattern must
;;;;     generate RTL which needs no reloading and needs no temporary
;;;;     registers--no registers other than the operands. For example, if you
;;;;     support the pattern with a define_expand, then in such a case the
;;;;     define_expand mustn't call force_reg or any other such function
;;;;     which might generate new pseudo registers. This requirement
;;;;     exists even for subword modes on a RISC machine where fetching
;;;;     those modes from memory normally requires several insns and
;;;;     some temporary registers. Look in `spur.md' to see how the
;;;;     requirement can be satisfied. During reload a memory reference
;;;;     with an invalid address may be passed as an operand. Such an
;;;;     address will be replaced with a valid address later in the reload
;;;;     pass. In this case, nothing may be done with the address except to
;;;;     use it as it stands. If it is copied, it will not be replaced with a valid
;;;;     address. No attempt should be made to make such an address into a
;;;;     valid address and no routine (such as change_address) that will do
;;;;     so may be called. Note that general_operand will fail when applied
;;;;     to such an address. The global variable reload_in_progress
;;;;     (which must be explicitly declared if required) can be used to
;;;;     determine whether such special handling is required. The variety of
;;;;     operands that have reloads depends on the rest of the machine
;;;;     description, but typically on a RISC machine these can only be
;;;;     pseudo registers that did not get hard registers, while on other
;;;;     machines explicit memory references will get optional reloads. If a
;;;;     scratch register is required to move an object to or from memory, it
;;;;     can be allocated using gen_reg_rtx prior to life analysis. If there
;;;;     are cases needing scratch registers after reload, you must define
;;;;     SECONDARY_INPUT_RELOAD_CLASS and perhaps also
;;;;     SECONDARY_OUTPUT_RELOAD_CLASS to detect them, and provide
;;;;     patterns `reload_inm' or `reload_outm' to handle them. See
;;;;     section Register Classes. The global variable no_new_pseudos can
;;;;     be used to determine if it is unsafe to create new pseudo registers.
;;;;     If this variable is nonzero, then it is unsafe to call gen_reg_rtx to
;;;;     allocate a new pseudo. The constraints on a `movm' must permit
;;;;     moving any hard register to any other hard register provided that
;;;;     HARD_REGNO_MODE_OK permits mode m in both registers and
;;;;     REGISTER_MOVE_COST applied to their classes returns a value of 2. It
;;;;     is obligatory to support floating point `movm' instructions into and
;;;;     out of any registers that can hold fixed point values, because unions
;;;;     and structures (which have modes SImode or DImode) can be in
;;;;     those registers and they may have floating point members. There
;;;;     may also be a need to support fixed point `movm' instructions in
;;;;     and out of floating point registers. Unfortunately, I have forgotten
;;;;     why this was so, and I don't know whether it is still true. If
;;;;     HARD_REGNO_MODE_OK rejects fixed point values in floating point
;;;;     registers, then the constraints of the fixed point `movm'
;;;;     instructions must be designed to avoid ever trying to reload into a
;;;;     floating point register. 

(define_expand "movpqi"
  [
   (set (match_operand:PQI 0 "nonimmediate_operand" "")
	(match_operand:PQI 1 "general_operand" "cgmi*yf"))
  ]
  ""
  "")

(define_expand "movqi"
    [
     (set   (match_operand:QI 0 "nonimmediate_operand" "")
            (match_operand:QI 1 "general_operand" ""))
    ]
  ""
  "if (xap2plus_expand_movqi(operands)) DONE;")

(define_expand "movhi"
    [
     (set   (match_operand:HI 0 "nonimmediate_operand" "")
            (match_operand:HI 1 "general_operand" ""))
    ]
  ""
  "if (xap2plus_expand_movhi(operands)) DONE;")

(define_insn "*movqi0"
  [
   (set (reg:QI RXL)
	(mem:QI (plus:QI (reg:QI RXL) (match_operand:QI 0 "immediate_operand" "i"))))
  ]
  ""
  "*
{
operands[1] = gen_rtx_REG(QImode, RXL);
operands[2] = gen_rtx_PLUS(QImode, operands[1], operands[0]);
operands[3] = gen_rtx_MEM(QImode, operands[2]);
return \"ld\\t%1,%3\";
} ")

;; Fix up dodgy sequence produced by movhi expansion
(define_peephole
  [
    (set (match_operand:QI 0 "hard_register_operand" "d")   ; eg, AH <- const
         (match_operand:QI 1 "immediate_operand" "i"))
    (set (match_operand:QI 2 "hard_register_operand" "d")   ; eg, AL <- AH
         (match_dup 0))
    (set (match_dup 0)                                      ; eg, AH <- const
         (match_operand:QI 3 "immediate_operand" "i"))
  ]
  ""
  "*
    return \"ld\\t%2,%1\;ld\\t%0,%3\";
  "
)

(define_insn "*movqi2"
    [
     (set   (match_operand:QI 0 "nonimmediate_operand" "=gm*yf,c,?c,?m")
            (match_operand:QI 1 "register_operand" "c,gmy*f,?c,?f"))
    ]
  ""
  "*
   {
    operands[2] = gen_rtx_REG(QImode, RSCRATCH);
    operands[3] = gen_rtx_REG(QImode, RAL);
    if(hard_register_operand(operands[0],QImode))
    {
        if(hard_register_operand(operands[1],QImode))
        {
            if(XAP_FLAG_ENABLED(ENHANCED))
                return \"ld\\t%0,%f1\";
            else
                return \"st\\t%1,%2\;ld\\t%0,%2\";
        }
        else
            return \"ld\\t%0,%1\";
     }
     else
     {
        if(hard_register_operand(operands[1],QImode))
           return \"st\\t%1,%0\";
        else
            return \"st\\t%3,%2\;ld\\t%3,%1\;st\\t%3,%0\;ld\\t%3,%2\";
     }
   }
")

(define_insn "*movqi3"
    [
     (set   (match_operand:QI 0 "hard_register_operand" "=c,?c")
            (match_operand:QI 1 "general_operand" "gmi*yf,?c"))
    ]
  ""
  "*
{
operands[2] = gen_rtx_REG(QImode, RSCRATCH);
switch(which_alternative)
   {
   case 0: return  \"ld\\t%0,%f1\";
   case 1: return  \"st\\t%1,%2\;ld\\t%0,%2\";
   default: abort();
   }
}")

(define_insn "*movqi4"
    [
     (set   (match_operand:QI 0 "register_operand" "+c,?c")
            (subreg:QI (match_operand:HI 1 "general_operand" "gmi*yf,?c") 0))
    ]
  ""
  "*
{
operands[2] = gen_rtx_REG(QImode, RSCRATCH);
switch(which_alternative)
   {
   case 0: return  \"ld\\t%0,%t1\";
   case 1: return  \"st\\t%t1,%2\;ld\\t%0,%2\";
   default: abort();
   }
}")

(define_insn "*movqi5"
    [
     (set   (match_operand:QI 0 "register_operand" "+c,?c")
            (subreg:QI (match_operand:HI 1 "general_operand" "gmi*yf,?c") 1))
    ]
  ""
  "*
{
operands[2] = gen_rtx_REG(QImode, RSCRATCH);
switch(which_alternative)
   {
   case 0: return  \"ld\\t%0,%b1\";
   case 1: return  \"st\\t%b1,%2\;ld\\t%0,%2\";
   default: abort();
   }
}")

(define_insn "*movqi6"
    [
     (set   (subreg:QI (match_operand:HI 0 "nonimmediate_operand" "=c,gm*yf,c") 0)
            (match_operand:QI 1 "register_operand" "gmi*yf,c,c"))
    ]
  ""
  "*switch(which_alternative)
   {
   case 0: return  \"ld\\t%t0,%f1\";
   case 1: 
           if (hard_register_operand(operands[0],QImode))
                return \"ld\\t%t0,%f1\";
           else
                return \"st\\t%1,%t0\";
   case 2: return  \"ld\\t%t0,%f1\";
   default: abort();
   }")

(define_insn "*movqi7"
    [
     (set   (subreg:QI (match_operand:HI 0 "nonimmediate_operand" "=c,gm*yf,c") 1)
            (match_operand:QI 1 "register_operand" "gmi*yf,c,c"))
    ]
  ""
  "*switch(which_alternative)
   {
   case 0: return  \"ld\\t%b0,%f1\";
   case 1: 
           if (hard_register_operand(operands[0],QImode))
                return \"ld\\t%b0,%f1\";
           else
                return \"st\\t%1,%b0\";
   default: abort();
   }")

(define_insn "*movqi8"
    [
     (set   (match_operand:QI 0 "register_operand" "=c")
            (match_operand:QI 1 "general_operand" "mi"))
    ]
  ""
  "*return \"ld\\t%0,%f1\";")

(define_insn "*movhi"
    [
     (set   (match_operand:HI 0 "nonimmediate_operand" "=y,d,c,gmfy,c")
            (match_operand:HI 1 "general_operand" "d,y,gmify,c,c"))
    ]
  ""
  "*
{
switch(which_alternative)
   {
   case 0:
   case 3:
           return  \"st\\t%t1,%t0\;st\\t%b1,%b0\";
   case 1:
   case 2:
   case 4:
           return  \"ld\\t%t0,%t1\;ld\\t%b0,%b1\";
   default:abort();
   }
}")

;;;; `addm3' 
;;;;     Add operand 2 and operand 1, storing the result in operand 0. All
;;;;     operands must have mode m. This can be used even on
;;;;     two-address machines, by means of constraints requiring operands
;;;;     1 and 0 to be the same location. 
;;;; `subm3', `mulm3'
;;;; `andm3', `iorm3', `xorm3' 
;;;;     Similar, for other arithmetic operations. 

(define_insn "*addhi3"
  [
     (set (match_operand:HI 0 "register_operand" "=c,c")
        (plus:HI (match_operand:HI 1 "register_operand" "%0,0")
                 (match_operand:HI 2 "general_operand" "gmify,!X")))
  ]
  ""
  "*
{
    operands[3] = GEN_INT(1);
    if (rtx_equal_p(operands[0],operands[2]))
        return \"asl\\t%3\";
    else
    {
        if ((GET_CODE(operands[2])==CONST_INT) && !(INTVAL(operands[2])&0xFFFF))
            return \"add\\t%t0,%t2\";
        else
            return \"add\\t%b0,%b2\;addc\\t%t0,%t2\";
    }
}")

(define_insn "*addhi3_ze1"
  [
     (set (match_operand:HI 0 "register_operand" "=c")
        (plus:HI (match_operand:HI 1 "register_operand" "0")
                 (zero_extend:HI (match_operand:QI 2 "general_operand" "gmify"))))
  ]
  ""
  "*
{
    operands[3] = const0_rtx;
    if ((GET_CODE(operands[2])==CONST_INT) && !(INTVAL(operands[2])&0xFFFF))
        return \"add\\t%t0,%f2\";
    else
        return \"add\\t%b0,%f2\;addc\\t%t0,%3\";
}")

(define_insn "*andhi3"
  [
     (set (match_operand:HI 0 "register_operand" "=c,c")
        (and:HI  (match_operand:HI 1 "register_operand" "%0,0")
                 (match_operand:HI 2 "general_operand" "gmi?yf,!X")))
  ]
  ""
  "*
{
if ((GET_CODE(operands[2])==CONST_INT) && ((INTVAL(operands[2])&0xFFFF)==0xFFFF))
   return \"and\\t%t0,%t2\";
else if ((GET_CODE(operands[2])==CONST_INT) && ((INTVAL(operands[2])&0xFFFF0000)==0xFFFF0000))
   return \"and\\t%b0,%b2\";
else
   return \"and\\t%b0,%b2\;and\\t%t0,%t2\";
}")
   
(define_insn "*iorhi3"
  [
     (set (match_operand:HI 0 "register_operand" "=c,c")
        (ior:HI  (match_operand:HI 1 "register_operand" "%0,0")
                 (match_operand:HI 2 "general_operand" "gmi?yf,!X")))
  ]
  ""
  "*
{
if ((GET_CODE(operands[2])==CONST_INT) && ((INTVAL(operands[2])&0xFFFF)==0x0))
   return \"or\\t%t0,%t2\";
else if ((GET_CODE(operands[2])==CONST_INT) && ((INTVAL(operands[2])&0xFFFF0000)==0x0))
   return \"or\\t%b0,%b2\";
else
   return \"or\\t%b0,%b2\;or\\t%t0,%t2\";
}")

(define_insn "*xorhi3"
  [
     (set (match_operand:HI 0 "register_operand" "=c,c")
        (xor:HI  (match_operand:HI 1 "general_operand" "%0,0")
                 (match_operand:HI 2 "general_operand" "gmi?yf,!X")))
  ]
  ""
  "*
{
if ((GET_CODE(operands[2])==CONST_INT) && ((INTVAL(operands[2])&0xFFFF)==0x0))
   return \"xor\\t%t0,%t2\";
else if ((GET_CODE(operands[2])==CONST_INT) && ((INTVAL(operands[2])&0xFFFF0000)==0x0))
   return \"xor\\t%b0,%b2\";
else
   return \"xor\\t%b0,%b2\;xor\\t%t0,%t2\";
}")

(define_insn "*subhi3"
  [
     (set (match_operand:HI 0 "register_operand" "=c,c")
        (minus:HI (match_operand:HI 1 "register_operand" "0,0")
                  (match_operand:HI 2 "general_operand" "gmify,X")))
  ]
  ""
  "*
{
if ((GET_CODE(operands[2])==CONST_INT) && ((INTVAL(operands[2])&0xFFFF)==0x0))
   return \"sub\\t%t0,%t2\";
else
   return \"sub\\t%b0,%b2\;subc\\t%t0,%t2\";
}")

(define_insn "*subhi3_ze"
  [
     (set (match_operand:HI 0 "register_operand" "=c")
        (minus:HI (match_operand:HI 1 "register_operand" "0")
                  (zero_extend:HI (match_operand:QI 2 "general_operand" "gmify"))))
  ]
  ""
  "*
  {
      operands[3] = const0_rtx;
      return  \"sub\\t%b0,%f2\;subc\\t%t0,%3\";
  }")

(define_expand "subsi3"
  [
      (set (match_operand:SI 0 "nonimmediate_operand" "=mfy")
           (minus:SI (match_operand:SI 1 "general_operand" "mfy")
                     (match_operand:SI 2 "general_operand" "mfy")))
  ]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__subsi3\");
  
  emit_library_call_value(xap_libcall, operands[0], 1, SImode, 2,
                          operands[1], SImode,
                          operands[2], SImode);
  DONE;
}"
)

(define_expand "addsi3"
  [
      (set (match_operand:SI 0 "nonimmediate_operand" "")
           (plus (match_operand:SI 1 "general_operand" "")
                 (match_operand:SI 2 "general_operand" "")))
  ]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__addsi3\");
  
  emit_library_call_value(xap_libcall, operands[0], 1, SImode, 2,
                          operands[1], SImode,
                          operands[2], SImode);
  DONE;
} "
)

(define_expand "addhi3"
    [
   (set (match_operand:HI 0 "register_operand" "")
        (plus:HI (match_operand:HI 1 "register_operand" "")
                 (match_operand:HI 2 "general_operand" "")))
    ]
  ""
  "if (xap2plus_expand_hi3(PLUS, operands)) DONE;")

(define_expand "andhi3"
  [
     (set (match_operand:HI 0 "register_operand" "")
        (and:HI  (match_operand:HI 1 "register_operand" "")
                 (match_operand:HI 2 "general_operand" "")))
  ]
  ""
  "if (!XAP_FLAG_ENABLED(ENHANCED) && xap2plus_expand_hi3(AND,operands)) DONE;")

(define_expand "iorhi3"
  [
     (set (match_operand:HI 0 "register_operand" "")
        (ior:HI  (match_operand:HI 1 "register_operand" "")
                 (match_operand:HI 2 "general_operand" "")))
  ]
  ""
  "if (!XAP_FLAG_ENABLED(ENHANCED) && xap2plus_expand_hi3(IOR,operands)) DONE;")

(define_expand "xorhi3"
  [
     (set (match_operand:HI 0 "register_operand" "")
        (xor:HI  (match_operand:HI 1 "general_operand" "")
                 (match_operand:HI 2 "general_operand" "")))
  ]
  ""
  "if (!XAP_FLAG_ENABLED(ENHANCED) && xap2plus_expand_hi3(XOR,operands)) DONE;")

(define_expand "subhi3"
  [
     (set (match_operand:HI 0 "register_operand" "")
        (minus:HI (match_operand:HI 1 "register_operand" "")
                  (match_operand:HI 2 "general_operand" "")))
  ]
  ""
  "")

(define_insn "*addqi4"
  [
   (set (match_operand:QI 0 "register_operand" "=c")
        (plus:QI (plus:QI (match_operand:QI 1 "register_operand" "cgm*yf")
                          (match_operand:QI 2 "immediate_operand" "i"))
                 (match_operand:QI 3 "general_operand" "cgmi*yf")))
  ]
  ""
  "*
{
if (XAP_FLAG_ENABLED(ENHANCED))
    {
    if (rtx_equal_p(operands[0],operands[1]))
        return \"add\\t%0,%f3\;add\\t%0,%f2\";
    if (rtx_equal_p(operands[0],operands[3]))
        return \"add\\t%0,%f1\;add\\t%0,%f2\";
    return \"ld\\t%0,%f3\;add\\t%0,%f1\;add\\t%0,%f2\";
    }
else
    {
    operands[4] = gen_rtx_REG(QImode, RSCRATCH);
    if (rtx_equal_p(operands[0],operands[1]))
        {
        if (hard_register_operand(operands[3], QImode))
            return \"st\\t%3,%4\;add\\t%0,%4\;add\\t%0,%2\";
        else
            return \"add\\t%0,%3\;add\\t%0,%2\";
        }
    if (rtx_equal_p(operands[0],operands[3]))
        {
        if (hard_register_operand(operands[1], QImode))
            return \"st\\t%1,%4\;add\\t%0,%4\;add\\t%0,%2\";
        else
            return \"add\\t%0,%1\;add\\t%0,%2\";
        }
    else
        {
        if (hard_register_operand(operands[1], QImode) && hard_register_operand(operands[3], QImode))
            return \"st\\t%3,%4\;ld\\t%0,%4\;st\\t%1,%4\;add\\t%0,%4\;add\\t%0,%2\";
        else if (hard_register_operand(operands[1], QImode))
            return \"ld\\t%0,%3\;st\\t%1,%4\;add\\t%0,%4\;add\\t%0,%2\";
        else if (hard_register_operand(operands[3], QImode))
            return \"st\\t%3,%4\;ld\\t%0,%4\;add\\t%0,%1\;add\\t%0,%2\";
        else
            return \"ld\\t%0,%3\;add\\t%0,%1\;add\\t%0,%2\";
        }
    
    }
}")

(define_insn "*addqi0"
  [
   (set (match_operand:QI 0 "register_operand" "=c")
        (plus:QI (reg:QI RFP)
                 (match_operand:QI 1 "immediate_operand" "i")))
  ]
  ""
  "*
{
    operands[2] = gen_rtx_REG(QImode, RFP);
    operands[3] = gen_rtx_REG(QImode, RSCRATCH);
    return XAP_FLAG_ENABLED(ENHANCED) ? \"ld\\t%0,%f2\;add\\t%0,%f1\" : \"st\\t%2,%3\;ld\\t%0,%3\;add\\t%0,%1\";
}")

(define_insn "*addqi2"
  [
   (set (match_operand:QI 0 "register_operand" "=c,c,c")
        (plus:QI (match_operand:QI 1 "register_operand" "0,0,y*f")
                (match_operand:QI 2 "general_operand" "gmify,0,gmify")))
  ]
  ""
  "*
{
operands[3] = gen_rtx_REG(QImode, RSCRATCH);
switch (which_alternative)
    {
    case 0: return  \"add\\t%0,%f2\";
    case 1: return  \"st\\t%1,%3\;add\\t%0,%3\";
    case 2:
        if(rtx_equal_p(operands[0],operands[2]))
            return  \"add\\t%0,%f1\";
        else
            return  \"ld\\t%0,%f2\;add\\t%0,%f1\";
    default: abort();
    }
}")

(define_expand "addqi3"
    [
   (set (match_operand:QI 0 "register_operand" "")
        (plus:QI (match_operand:QI 1 "register_operand" "")
                 (match_operand:QI 2 "general_operand" "")))
    ]
  ""
  "if (xap2plus_expand_qi3(PLUS, operands)) DONE;")

(define_insn "*andqi3"
  [
   (set (match_operand:QI 0 "register_operand" "=c")
        (and:QI (match_operand:QI 1 "register_operand" "%0")
                 (match_operand:QI 2 "general_operand" "gmi*yf")))
  ]
  ""
  "*return  \"and\\t%0,%f2\"; ")

(define_expand "andqi3"
  [
     (set (match_operand:QI 0 "register_operand" "")
        (and:QI  (match_operand:QI 1 "register_operand" "")
                 (match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_qi3(AND, operands)) DONE;")

(define_insn "*iorqi3"
  [
   (set (match_operand:QI 0 "register_operand" "=c")
        (ior:QI (match_operand:QI 1 "register_operand" "%0")
                 (match_operand:QI 2 "general_operand" "gmi*yf")))
  ]
  ""
  "*return  \"or\\t%0,%f2\"; ")

(define_expand "iorqi3"
  [
     (set (match_operand:QI 0 "register_operand" "")
        (ior:QI  (match_operand:QI 1 "register_operand" "")
                 (match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_qi3(IOR, operands)) DONE;")

(define_insn "*xorqi3"
  [
   (set (match_operand:QI 0 "register_operand" "=c")
        (xor:QI (match_operand:QI 1 "register_operand" "%0")
                 (match_operand:QI 2 "general_operand" "gmi*yf")))
  ]
  ""
  "*return  \"xor\\t%0,%f2\"; ")

(define_expand "xorqi3"
  [
     (set (match_operand:QI 0 "register_operand" "")
        (xor:QI  (match_operand:QI 1 "register_operand" "")
                 (match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_qi3(XOR, operands)) DONE;")

(define_insn "*subqi3"
  [
   (set (match_operand:QI 0 "register_operand" "=c,c")
        (minus:QI (match_operand:QI 1 "register_operand" "0,gmi*yf")
                 (match_operand:QI 2 "general_operand" "gmif,0")))
  ]
  ""
  "*switch(which_alternative)
   {
   case 0: return  \"sub\\t%0,%f2\";
   case 1: return  \"nadd\\t%0,%f1\";
   default: abort();
   }")

(define_expand "subqi3"
  [
     (set (match_operand:QI 0 "register_operand" "")
        (minus:QI  (match_operand:QI 1 "register_operand" "")
                 (match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_qi3(MINUS, operands)) DONE;")

;;;; `divm3', `udivm3', `modm3', `umodm3' 
;;;; `sminm3', `smaxm3', `uminm3', `umaxm3' 

(define_insn "*udivqi3_const"
  [
    (set (reg:QI RAL)
         (subreg:QI (udiv:HI 
            (reg:HI RAH)
            (match_operand:QI 0 "immediate_operand" "i")) 1))
  ]
  ""
  "*return  \"udiv\\t%0\"; ")

(define_insn "*divqi3_const"
  [
    (set (reg:QI RAL)
         (subreg:QI (div:HI 
            (reg:HI RAH)
            (match_operand:QI 0 "immediate_operand" "i")) 1))
  ]
  ""
  "*return  \"sdiv\\t%0\"; ")

(define_insn "*umodqi3_const"
  [
    (set (reg:QI RAH)
         (subreg:QI (umod:HI 
            (reg:HI RAH)
            (match_operand:QI 0 "immediate_operand" "i")) 1))
  ]
  ""
  "*return  \"udiv\\t%0\"; ")

(define_insn "*modqi3_const"
  [
    (set (reg:QI RAH)
         (subreg:QI (mod:HI 
            (reg:HI RAH)
            (match_operand:QI 0 "immediate_operand" "i")) 1))
  ]
  ""
  "*return  \"sdiv\\t%0\"; ")

(define_insn "*udivqi3"
  [
    (set (reg:QI RAL)
         (subreg:QI (udiv:HI 
            (reg:HI RAH)
            (zero_extend:HI (match_operand:QI 0 "general_operand" "gmif,?c"))) 1))
  ]
  ""
  "*
{
operands[1] = gen_rtx_REG(QImode, RSCRATCH);
if (XAP_FLAG_ENABLED(ENHANCED) || !which_alternative) return \"udiv\\t%f0\";
return \"st\\t%0,%1\;udiv\\t%1\";
}")

(define_insn "*divqi3"
  [
    (set (reg:QI RAL)
         (subreg:QI (div:HI 
            (reg:HI RAH)
            (sign_extend:HI (match_operand:QI 0 "general_operand" "gmify,?c"))) 1))
  ]
  ""
  "*
{
operands[1] = gen_rtx_REG(QImode, RSCRATCH);
if (XAP_FLAG_ENABLED(ENHANCED) || !which_alternative) return \"sdiv\\t%f0\";
return \"st\\t%0,%1\;sdiv\\t%1\";
}")

(define_insn "*umodqi3"
  [
    (set (reg:QI RAH)
         (subreg:QI (umod:HI 
            (reg:HI RAH)
            (zero_extend:HI (match_operand:QI 0 "general_operand" "gmif,?c"))) 1))
  ]
  ""
  "*
{
operands[1] = gen_rtx_REG(QImode, RSCRATCH);
if (XAP_FLAG_ENABLED(ENHANCED) || !which_alternative) return \"udiv\\t%f0\";
return \"st\\t%0,%1\;udiv\\t%1\";
}")

(define_insn "*modqi3"
  [
    (set (reg:QI RAH)
         (subreg:QI (mod:HI 
            (reg:HI RAH)
            (sign_extend:HI (match_operand:QI 0 "general_operand" "gmify,?c"))) 1))
  ]
  ""
  "*
{
operands[1] = gen_rtx_REG(QImode, RSCRATCH);
if (XAP_FLAG_ENABLED(ENHANCED) || !which_alternative) return \"sdiv\\t%f0\";
return \"st\\t%0,%1\;sdiv\\t%1\";
}")

(define_expand "udivqi3"
  [
     (set (match_operand:QI 0 "nonimmediate_operand" "=cgm*yf")
	(udiv:QI (match_operand:QI 1 "register_operand" "cgmi*yf")
		 (match_operand:QI 2 "general_operand" "cgmi*yf")))
  ]
  ""
  "if (xap2plus_expand_div_qi3(UDIV, ZERO_EXTEND, operands)) DONE;")

(define_expand "umodqi3"
  [
     (set (match_operand:QI 0 "register_operand" "=c")
	(umod:QI (match_operand:QI 1 "register_operand" "%0")
		 (match_operand:QI 2 "general_operand" "gmi*yf")))
  ]
  ""
  "if (xap2plus_expand_div_qi3(UMOD, ZERO_EXTEND, operands)) DONE;")

(define_expand "divqi3"
  [
     (set (match_operand:QI 0 "nonimmediate_operand" "=cgm*yf")
	(div:QI (match_operand:QI 1 "register_operand" "cgmi*yf")
		 (match_operand:QI 2 "general_operand" "cgmi*yf")))
  ]
  ""
  "if (xap2plus_expand_div_qi3(DIV, SIGN_EXTEND, operands)) DONE;")

(define_expand "modqi3"
  [
     (set (match_operand:QI 0 "nonimmediate_operand" "=cgm*yf")
	(mod:QI (match_operand:QI 1 "register_operand" "cgmi*yf")
		 (match_operand:QI 2 "general_operand" "cgmi*yf")))
  ]
  ""
  "if (xap2plus_expand_div_qi3(MOD, SIGN_EXTEND, operands)) DONE;")

;;;; `mulsisi3' 
;;;;     Multiply operands 1 and 2, wsich have mode HImode, and store a
;;;;     SImode product in operand 0. 
;;;; `mulhisi3', `mulsidi3' 
;;;;     Similar widening-multiplication instructions of other widths. 
;;;; `umulhisi3', `umulsisi3', `umulsidi3' 
;;;;     Similar widening-multiplication instructions that do unsigned
;;;;     multiplication. 
;;;; `smulm3_sighpart' 
;;;;     Perform a signed multiplication of operands 1 and 2, wsich have
;;;;     mode m, and store the most significant half of the product in
;;;;     operand 0. The least significant half of the product is discarded. 
;;;; `umulm3_sighpart' 
;;;;     Similar, but the multiplication is unsigned. 

(define_insn "*mulqihi3_const"
  [
    (set (reg:HI RAH)
         (mult:HI 
            (sign_extend:HI (reg:QI RAL))
            (match_operand:QI 0 "immediate_operand" "i")))
  ]
  ""
  "*
  {
    int m = INTVAL(operands[0]) & 0xFFFF;
    int c;
    for(c = 0; c <= 14; ++c)
        if(m == (1<<c))
        {
            operands[1] = GEN_INT(16);
            operands[2] = GEN_INT(16-c);
            return \"lsl\t%1\;asr\t%2\";
        }

    return  \"smult\\t%0\";
  }
  ")

(define_insn "*umulqihi3_const"
  [
    (set (reg:HI RAH)
         (mult:HI 
            (zero_extend:HI (reg:QI RAL))
            (match_operand:QI 0 "immediate_operand" "i")))
  ]
  ""
  "*
  {
    int m = INTVAL(operands[0]) & 0xFFFF;
    int c;
    for(c = 0; c <= 15; ++c)
        if(m == (1<<c))
        {
            operands[1] = GEN_INT(c);
            operands[2] = gen_rtx_REG(QImode,RAH);
            operands[3] = const0_rtx;
            return \"ld\\t%2,%3\;lsl\\t%1\";
        }
    
    return  \"umult\\t%0\";
  }
  ")

(define_insn "*mulqihi3"
  [
    (set (reg:HI RAH)
         (mult:HI 
            (sign_extend:HI (reg:QI RAL))
            (sign_extend:HI (match_operand:QI 0 "general_operand" "gmify"))))
  ]
  ""
  "*
{
operands[1] = gen_rtx_REG(QImode, RSCRATCH);
if (GET_CODE(operands[0])==REG && REGNO(operands[0])<2)
{
    return \"st\\t%0,%1\;smult\\t%1\";
}
else
{
    return \"smult\\t%f0\";
}
}")

(define_insn "*umulqihi3"
  [
    (set (reg:HI RAH)
         (mult:HI 
            (zero_extend:HI (reg:QI RAL))
            (zero_extend:HI (match_operand:QI 0 "general_operand" "gmify"))))
  ]
  ""
  "*
{
operands[1] = gen_rtx_REG(QImode, RSCRATCH);
if (GET_CODE(operands[0])==REG && REGNO(operands[0])<2)
{
    return \"st\\t%0,%1\;umult\\t%1\";
}
else
{
    return \"umult\\t%f0\";
}
}")

(define_expand "mulqi3"
  [
     (set (match_operand:QI 0 "register_operand" "")
        (mult:QI (match_operand:QI 1 "register_operand" "")
                 (match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_mult_qi3(MULT, SIGN_EXTEND, operands)) DONE;")

(define_expand "mulqihi3"
  [
    (set (match_operand:HI 0 "nonimmediate_operand" "")
         (mult:HI 
            (sign_extend:HI (match_operand:QI 1 "register_operand" ""))
            (sign_extend:HI (match_operand:QI 2 "general_operand" ""))))
  ]
  ""
  "if (xap2plus_expand_mult_qi3(MULT, SIGN_EXTEND, operands)) DONE;")

(define_expand "mulhi3"
  [
    (set (match_operand:HI 0 "nonimmediate_operand" "")
         (mult:HI (match_operand:HI 1 "general_operand" "")
                  (match_operand:HI 2 "general_operand" "")
         ))
  ]
  ""
  "
  if(GET_CODE(operands[2])==CONST_INT)
  {
    int m = INTVAL(operands[2]);
    int c;
    for(c=0;c<=30;++c)
    {
        if(m == (1<<c))
        {
            emit_insn(gen_rtx_SET(VOIDmode,operands[0],gen_rtx_ASHIFT(HImode,operands[1],GEN_INT(c))));

            DONE;
        }
        else if(m == -(1<<c))
        {
            rtx temp = gen_reg_rtx(HImode);
            emit_insn(gen_rtx_SET(VOIDmode,temp,gen_rtx_ASHIFT(HImode,operands[1],GEN_INT(c))));
            emit_insn(gen_rtx_SET(VOIDmode,operands[0],gen_rtx_NEG(HImode,temp)));

            DONE;
        }
        
    }
    
    /* fall through if we can't just use shifts */
  }


  { /* result = a1a0 * b1b0 */
    rtx a0 = gen_reg_rtx(QImode);
    rtx a1 = gen_reg_rtx(QImode);
    rtx b0 = gen_reg_rtx(QImode);
    rtx b1 = gen_reg_rtx(QImode);
    rtx tmp = gen_reg_rtx(QImode);
    rtx al = gen_rtx_REG(QImode,RAL);
    rtx ah = gen_rtx_REG(QImode,RAH);
    rtx ahal = gen_rtx_REG(HImode,RAH);
    rtx ze_al, ze_b0, se_al, se_b0, se_b1;

    a0 = simplify_gen_subreg(QImode,operands[1],HImode,1);
    a1 = simplify_gen_subreg(QImode,operands[1],HImode,0);
    b0 = simplify_gen_subreg(QImode,operands[2],HImode,1);
    b1 = simplify_gen_subreg(QImode,operands[2],HImode,0);

    ze_al = gen_rtx_ZERO_EXTEND(HImode,al);
    se_al = gen_rtx_SIGN_EXTEND(HImode,al);
    
    if(GET_CODE(operands[2])==CONST_INT)
    {
        se_b0 = GEN_INT( (INTVAL(operands[2])<<16)>>16 );
        ze_b0 = se_b0;
        se_b1 = GEN_INT( INTVAL(operands[2])>>16 );
    }
    else
    {
        ze_b0 = gen_rtx_ZERO_EXTEND(HImode,b0);
        se_b0 = gen_rtx_SIGN_EXTEND(HImode,b0);
        se_b1 = gen_rtx_SIGN_EXTEND(HImode,b1);
    }

    if(GET_CODE(se_b1)==CONST_INT && INTVAL(se_b1)==0)
    {
        emit_insn(gen_rtx_SET(VOIDmode, tmp, const0_rtx));
    }
    else
    {
        emit_insn(gen_rtx_SET(VOIDmode, al , a0));
        emit_insn(gen_rtx_SET(VOIDmode, ahal, gen_rtx_MULT(HImode,se_al,se_b1)));
        emit_insn(gen_rtx_SET(VOIDmode, tmp, al));
    }
    
    if(! (GET_CODE(se_b0)==CONST_INT && INTVAL(se_b0)==0) )
    {
        emit_insn(gen_rtx_SET(VOIDmode, al , a1));
        emit_insn(gen_rtx_SET(VOIDmode, ahal, gen_rtx_MULT(HImode,se_al,se_b0)));
        emit_insn(gen_rtx_SET(VOIDmode, al, gen_rtx_PLUS(QImode,al,tmp)));
        emit_insn(gen_rtx_SET(VOIDmode, tmp, al));
    }
    
    if(GET_CODE(ze_b0)==CONST_INT && INTVAL(ze_b0)==0)
    {
        emit_insn(gen_rtx_SET(VOIDmode, al, const0_rtx));
        emit_insn(gen_rtx_SET(VOIDmode, ah, tmp));
    }
    else
    {
        emit_insn(gen_rtx_SET(VOIDmode, al, a0));
        emit_insn(gen_rtx_SET(VOIDmode, ahal, gen_rtx_MULT(HImode,ze_al,ze_b0)));
        emit_insn(gen_rtx_SET(VOIDmode, ah, gen_rtx_PLUS(QImode,ah,tmp)));
    }
    emit_insn(gen_rtx_SET(VOIDmode, operands[0], ahal));

    DONE;
  }
")

(define_insn "*mulqi3_const"
  [
    (set (match_operand:QI 0 "register_operand" "=d")
         (subreg:QI (mult:HI 
            (sign_extend:HI (reg:QI RAL))
            (match_operand:QI 1 "immediate_operand" "i")) 1))
    (clobber (reg:QI RAH))
  ]
  ""
  "*
  {
    int m = INTVAL(operands[1]) & 0xFFFF;
    int c;
    operands[2] = gen_rtx_REG(QImode,RAL);
    operands[3] = GEN_INT(16);
    operands[5] = GEN_INT(0);
    for(c = 0; c <= 14; ++c)
    {
        operands[4] = GEN_INT(16-c);
        if(m == (1<<c))
            return \"lsl\t%3\;asr\t%4\";
        else if(m == (-(1<<c) & 0xFFFF))
            return \"lsl\t%3\;asr\t%4\;nadd\t%2,%5\";
    }

    return  \"smult\\t%1\";
  }
  ")

(define_insn "*mulqi3_const_1"
  [
    (set (match_operand:HI 0 "register_operand" "=d")
         (mult:HI 
            (sign_extend:HI (reg:QI RAL))
            (match_operand:QI 1 "immediate_operand" "i")) )
  ]
  ""
  "*
  {
    int m = INTVAL(operands[1]) & 0xFFFF;
    int c;
    operands[2] = gen_rtx_REG(QImode,RAL);
    operands[3] = GEN_INT(16);
    operands[5] = GEN_INT(0);
    for(c = 0; c <= 14; ++c)
    {
        operands[4] = GEN_INT(16-c);
        if(m == (1<<c))
            return \"lsl\t%3\;asr\t%4\";
        else if(m == (-(1<<c) & 0xFFFF))
            return \"lsl\t%3\;asr\t%4\;nadd\t%2,%5\";
    }

    return  \"smult\\t%1\";
  }
  ")

(define_insn "*umulqi3_const"
  [
    (set (match_operand:QI 0 "register_operand" "=d")
         (subreg:QI (mult:HI 
            (zero_extend:HI (reg:QI RAL))
            (match_operand:QI 1 "immediate_operand" "i")) 1))
    (clobber (reg:QI RAH))
  ]
  ""
  "*
  {
    int m = INTVAL(operands[1]) & 0xFFFF;
    int c;
    for(c = 0; c <= 15; ++c)
        if(m == (1<<c))
        {
            operands[1] = GEN_INT(c);
            return \"lsl\t%1\";
        }

    return  \"umult\\t%1\";
  }
")
(define_insn "*umulqi3_const_1"
  [
    (set (match_operand:HI 0 "register_operand" "=d")
         (mult:HI (zero_extend:HI (reg:QI RAL))
            (match_operand:QI 1 "immediate_operand" "i")))
  ]
  ""
  "*
  {
    int m = INTVAL(operands[1]) & 0xFFFF;
    int c;
    for(c = 0; c <= 15; ++c)
        if(m == (1<<c))
        {
            operands[1] = GEN_INT(c);
            return \"lsl\t%0\";
        }

    return  \"umult\\t%1\";
  }
")

(define_insn "*mulqi3"
  [
    (set (reg:QI RAL)
         (subreg:QI (mult:HI 
            (sign_extend:HI (reg:QI RAL))
            (sign_extend:HI (match_operand:QI 0 "general_operand" "gmf"))) 1))
    (clobber (reg:QI RAH))
  ]
  ""
  "*
{
    operands[1] = gen_rtx_REG(QImode, RSCRATCH);
    if(GET_CODE(operands[0])==REG
       && (REGNO(operands[0])==RAH || REGNO(operands[0])==RAL))
    {
            return  \"st\\t%0,%1\;smult\\t%1\";
    }
    else
        return \"smult\\t%f0\";
}")

(define_insn "*umulqi3"
  [
    (set (reg:QI RAL)
         (subreg:QI (mult:HI 
            (zero_extend:HI (reg:QI RAL))
            (zero_extend:HI (match_operand:QI 0 "general_operand" "gmf"))) 1))
    (clobber (reg:QI RAH))
  ]
  ""
  "*
  {
    operands[1] = gen_rtx_REG(QImode, RSCRATCH);
    if(GET_CODE(operands[0])==REG
       && (REGNO(operands[0])==RAH || REGNO(operands[0])==RAL))
    {
            return  \"st\\t%0,%1\;umult\\t%1\";
    }
    else
        return  \"umult\\t%0\";
  }")

(define_expand "umulqi3"
  [
     (set (match_operand:QI 0 "register_operand" "")
        (mult:QI (match_operand:QI 1 "register_operand" "")
                 (match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_mult_qi3(MULT, ZERO_EXTEND, operands)) DONE;")

(define_expand "umulqihi3"
  [
    (set (match_operand:HI 0 "nonimmediate_operand" "")
         (mult:HI 
            (zero_extend:HI (match_operand:QI 1 "register_operand" ""))
            (zero_extend:HI (match_operand:QI 2 "general_operand" ""))))
  ]
  ""
  "if (xap2plus_expand_mult_qi3(MULT, ZERO_EXTEND, operands)) DONE;")

(define_expand "udivhi3"
  [
     (set (match_operand:HI 0 "register_operand" "")
	(udiv:HI (match_operand:HI 1 "register_operand" "")
		 (match_operand:HI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_arith_hi3(UDIV, operands)) DONE;")

(define_expand "umodhi3"
  [
     (set (match_operand:HI 0 "nonimmediate_operand" "")
	(umod:HI (match_operand:HI 1 "register_operand" "")
		 (match_operand:HI 2 "general_operand" "")))]
  ""
  "if (xap2plus_expand_arith_hi3(UMOD, operands)) DONE;")

(define_expand "divhi3"
  [
     (set (match_operand:HI 0 "nonimmediate_operand" "=cgm*yf")
	(div:HI (match_operand:HI 1 "register_operand" "cgmi*yf")
		 (match_operand:HI 2 "general_operand" "cgmi*yf")))]
  ""
  "if (xap2plus_expand_arith_hi3(DIV, operands)) DONE;")

(define_expand "modhi3"
  [
     (set (match_operand:HI 0 "nonimmediate_operand" "=cgm*yf")
	(mod:HI (match_operand:HI 1 "register_operand" "cgmi*yf")
		 (match_operand:HI 2 "general_operand" "cgmi*yf")))]
  ""
  "if (xap2plus_expand_arith_hi3(MOD, operands)) DONE;")

;;;; `ashlm3' 
;;;;     Arithmetic-ssift operand 1 left by a number of bits specified by
;;;;     operand 2, and store the result in operand 0. Here m is the mode of
;;;;     operand 0 and operand 1; operand 2's mode is specified by the
;;;;     instruction pattern, and the compiler will convert the operand to
;;;;     that mode before generating the instruction. 

(define_expand "ashlqi3"
  [
    (set (match_operand:QI 0 "nonimmediate_operand" "")
         (ashift:QI (match_operand:QI 1 "general_operand" "")
                    (match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_shift_qi3(ASHIFT, operands)) DONE;")

(define_insn "*ashlhi3"
  [
    (set (match_operand:HI 0 "register_operand" "=d,d")
         (ashift:HI (match_operand:HI 1 "register_operand" "0,0")
                    (match_operand:QI 2 "general_operand" "gmif,?c")))
  ]
  ""
  "*
{
operands[3] = gen_rtx_REG(QImode, RSCRATCH);
if (XAP_FLAG_ENABLED(ENHANCED) || !which_alternative)
    return \"asl\\t%f2\";
else
    return \"st\\t%2,%3\;asl\\t%3\";
}")

(define_expand "ashlhi3"
  [
    (set (match_operand:HI 0 "nonimmediate_operand" "")
         (ashift:HI (match_operand:HI 1 "general_operand" "")
                    (match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_shift_hi3(ASHIFT, operands)) DONE;")

(define_expand "ashrqi3"
  [
    (set (match_operand:QI 0 "nonimmediate_operand" "")
	 (ashiftrt:QI 	(match_operand:QI 1 "general_operand" "")
			(match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_shift_qi3(ASHIFTRT, operands)) DONE;")

(define_insn "*ashrhi3"
  [
    (set (reg:HI RAH)
         (ashiftrt:HI (reg:HI RAH)
                      (match_operand:QI 0 "general_operand" "gmif,?c")))
  ]
  ""
  "*
{
operands[1] = gen_rtx_REG(QImode, RSCRATCH);
if (XAP_FLAG_ENABLED(ENHANCED) || !which_alternative)
    return \"asr\\t%f0\";
else
    return \"st\\t%0,%1\;asr\\t%1\";
}")

(define_expand "ashrhi3"
  [
    (set (match_operand:HI 0 "nonimmediate_operand" "")
	 (ashiftrt:HI 	(match_operand:HI 1 "general_operand" "")
			(match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_shift_hi3(ASHIFTRT, operands)) DONE;")

(define_expand "lshrqi3"
  [
    (set (match_operand:QI 0 "nonimmediate_operand" "")
         (lshiftrt:QI (match_operand:QI 1 "general_operand" "")
                      (match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_shift_qi3(LSHIFTRT, operands)) DONE;")

(define_insn "*extendqihi2"
  [
    (set (match_operand:HI 0 "register_operand" "=c,c,c")
         (sign_extend:HI (match_operand:QI 1 "register_operand" "d,?gmfy,!X")))
  ]
  ""
  "*
{
operands[2] = GEN_INT(16);
switch(REGNO(operands[1]))
    {
    case RAH: return \"asr\\t%2\";
    case RAL: return \"asl\\t%2\;asr\\t%2\";
    default: return \"ld\\t%0,%f1\;asr\\t%2\";
    }
} ")

(define_insn "*lshrqi3"
  [
    (set (reg:QI RAH)
         (lshiftrt:QI
           (reg:QI RAH)
           (match_operand:QI 0 "general_operand" "gmify,?c")))
    (clobber:QI (reg:QI RAL))
  ]
  ""
  "*
{
operands[1] = gen_rtx_REG(QImode, RSCRATCH);
if (XAP_FLAG_ENABLED(ENHANCED) || !which_alternative)
    return \"lsr\\t%f0\";
else
    return \"st\\t%0,%1\;lsr\\t%1\";
}")

(define_insn "*ashrqi3"
  [
    (set (reg:QI RAH)
         (ashiftrt:QI
           (reg:QI RAH)
           (match_operand:QI 0 "general_operand" "gmify,?c")))
    (clobber:QI (reg:QI RAL))
  ]
  ""
  "*
{
operands[1] = gen_rtx_REG(QImode, RSCRATCH);
if (XAP_FLAG_ENABLED(ENHANCED) || !which_alternative)
    return \"asr\\t%f0\";
else
    return \"st\\t%0,%1\;asr\\t%1\";
}")
   

(define_insn "*ashlqi3"
  [
    (set (reg:QI RAL)
         (ashift:QI
           (reg:QI RAL)
           (match_operand:QI 0 "general_operand" "gmify,?c")) )
    (clobber:QI (reg:QI RAH))
  ]
  ""
  "*
{
operands[1] = gen_rtx_REG(QImode, RSCRATCH);
if (XAP_FLAG_ENABLED(ENHANCED) || !which_alternative)
    return \"asl\\t%f0\";
else
    return \"st\\t%0,%1\;asl\\t%1\";
}")

(define_insn "*lshrhi3"
  [
    (set (reg:HI RAH)
	 (lshiftrt:HI 	(reg:HI RAH)
         			(match_operand:QI 0 "general_operand" "gmify,?c")))
  ]
  ""
  "*
{
operands[1] = gen_rtx_REG(QImode, RSCRATCH);
if (XAP_FLAG_ENABLED(ENHANCED) || !which_alternative)
    return \"lsr\\t%f0\";
else
    return \"st\\t%0,%1\;lsr\\t%1\";
}")

(define_expand "lshrhi3"
  [
    (set (match_operand:HI 0 "nonimmediate_operand" "")
         (lshiftrt:HI (match_operand:HI 1 "general_operand" "")
                      (match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "if (xap2plus_expand_shift_hi3(LSHIFTRT, operands)) DONE;")

;;;; `negm2' 
;;;;     Negate operand 1 and store the result in operand 0. 

(define_insn "negqi2"
  [(set (match_operand:QI 0 "register_operand" "=c")
	(neg:QI (match_operand:QI 1 "register_operand" "0")))]
  ""
  "*
{
operands[2] = const0_rtx;
return  \"nadd\\t%0,%2\";
}")

(define_insn "neghi2"
  [(set (match_operand:HI 0 "register_operand" "=c")
	(neg:HI (match_operand:HI 1 "general_operand" "0")))]
  ""
  "*
{
operands[2] = const0_rtx;
operands[3] = GEN_INT(-1);
return  \"nadd\\t%b0,%b2\;xor\\t%t0,%t3\;addc\\t%t0,%t2\";
}")

(define_insn "one_cmplqi2"
  [(set (match_operand:QI 0 "register_operand" "=c")
	(not:QI (match_operand:QI 1 "register_operand" "0")))]
  ""
  "*
{
operands[2] = GEN_INT(-1);
return  \"xor\\t%0,%2\";
}")

(define_insn "one_cmplhi2"
  [(set (match_operand:HI 0 "nonimmediate_operand" "=c")
	(not:HI (match_operand:HI 1 "general_operand" "0")))]
  ""
  "*
{
operands[2] = GEN_INT(-1);
return  \"xor\\t%b0,%b2\;xor\\t%t0,%t2\";
}")

;;;; `tstm'
;;;;     Compare operand 0 against zero, and set the condition codes. The
;;;;     RTL pattern should look like tsis: 
;;;; 
;;;;     (set (cc0) (match_operand:m 0 ...))
;;;; 
;;;;     `tstm' patterns should not be defined for macsines that do not use
;;;;     (cc0). Doing so would confuse the optimizer since it would no
;;;;     longer be clear wsich set operations were comparisons. The
;;;;     `cmpm' patterns should be used instead. 

(define_expand "tstqi"
  [
     (set (cc0) (match_operand:QI 0 "general_operand" ""))]
  ""
  "")

(define_expand "tsthi"
  [
     (set (cc0) (match_operand:HI 0 "general_operand" ""))]
  "0 && !XAP_FLAG_ENABLED(BIG_FUNCTION_PTR)"
  "")

(define_expand "extendqihi2"
  [
    (set (match_operand:HI 0 "register_operand" "")
         (sign_extend:HI (match_operand:QI 1 "register_operand" "")))
  ]
  ""
  "")

(define_expand "zero_extendqihi2"
    [
        (set (match_operand:HI 0 "register_operand" "")
             (zero_extend:HI (match_operand:QI 1 "general_operand" "")))
    ]
    ""
    ""
)

;;;; `zero_extendmn2' 
;;;;     Zero-extend operand 1 (valid for mode m) to mode n and store in
;;;;     operand 0 (wsich has mode n). Both modes must be fixed point. 

(define_insn "*zero_extendqihi2"
  [
    (set (match_operand:HI 0 "register_operand" "=d")
         (zero_extend:HI (match_operand:QI 1 "memory_operand" "gmfy")))
  ]
  ""
  "*
{ /* We know op0 must be AH:AL */
    operands[2] = const0_rtx;
    return \"ld\\t%t0,%2\;ld\\t%b0,%1\";
}")

(define_insn "*zero_extendqihi2_1"
  [
    (set (match_operand:HI 0 "register_operand" "=c,c")
         (zero_extend:HI (match_operand:QI 1 "register_operand" "c,?mfy")))
  ]
  ""
  "*
{ /* We know op0 must be AH:AL */
    operands[2] = const0_rtx;
    operands[4] = gen_rtx_REG (QImode, RSCRATCH);
    operands[5] = GEN_INT(16);
    switch(which_alternative)
    {
        case 0:
           if      (REGNO(operands[1])==RAL) return \"ld\\t%t0,%2\";
           else if  (REGNO(operands[1])==RAL) return \"lsr\\t%5\";
           else if (XAP_FLAG_ENABLED(ENHANCED)) return \"ld\\t%b0,%f1\;ld\\t%t0,%2\";
           else                 return \"st\\t%1,%4\;ld\\t%b0,%4\;ld\\t%t0,%2\";
        case 1:
           return \"ld\\t%t0,%2\;ld\\t%b0,%1\";
    }
}")

(define_insn "*zero_extendqihi2_2"
  [
    (set (match_operand:HI 0 "register_operand" "=d")
         (zero_extend:HI (match_operand:QI 1 "immediate_operand" "i")))
  ]
  ""
  "*
{ /* We know op0 must be AH:AL */
    operands[2] = const0_rtx;
    return \"ld\\t%t0,%2\;ld\\t%b0,%1\";
}")

(define_peephole
  [
    (set (match_operand:HI 0 "register_operand" "=d")
         (zero_extend:HI (match_operand:QI 1 "general_operand" "mfiy")))
    (set (match_dup 0)
         (ashift:HI (match_dup 0) (const_int 16)))
  ]
  "XAP_FLAG_ENABLED(ENHANCED)"
  "*
    operands[2] = const0_rtx;
    if(GET_CODE(operands[1])==REG && REGNO(operands[1])==RAH)
        return \"ld\\t%b0,%2\";
    else
        return \"ld\\t%t0,%f1\;ld\\t%b0,%2\";
  "
)

(define_expand "beq"
  [
     (set (pc) (if_then_else (eq (cc0) (const_int 0)) (label_ref (match_operand 0 "" "")) (pc)))]
  ""
  "if (xap2plus_expand_compare_reg(EQ, operands[0])) DONE; ")

(define_expand "bne"
  [
     (set (pc) (if_then_else (ne (cc0) (const_int 0)) (label_ref (match_operand 0 "" "")) (pc)))]
  ""
  "if (xap2plus_expand_compare_reg(NE, operands[0])) DONE; ")

(define_expand "blt"
  [
     (set (pc) (if_then_else (lt (cc0) (const_int 0)) (label_ref (match_operand 0 "" "")) (pc)))]
  ""
  "if (xap2plus_expand_compare_reg(LT, operands[0])) DONE; ")

(define_expand "bltu"
  [
     (set (pc) (if_then_else (ltu (cc0) (const_int 0)) (label_ref (match_operand 0 "" "")) (pc)))]
  ""
  "if (xap2plus_expand_compare_reg(LTU, operands[0])) DONE; ")

(define_expand "bgeu"
  [
     (set (pc) (if_then_else (geu (cc0) (const_int 0)) (label_ref (match_operand 0 "" "")) (pc)))]
  ""
  "if (xap2plus_expand_compare_reg(GEU, operands[0])) DONE; ")

(define_expand "ble"
  [
     (set (pc) (if_then_else (le (cc0) (const_int 0)) (label_ref (match_operand 0 "" "")) (pc)))]
  ""
  "if (xap2plus_expand_compare_reg(LE, operands[0])) DONE; ")

(define_expand "bleu"
  [
     (set (pc) (if_then_else (leu (cc0) (const_int 0)) (label_ref (match_operand 0 "" "")) (pc)))
  ]
  ""
  "if (xap2plus_expand_compare_reg(LEU, operands[0])) DONE; ")

(define_expand "bgt"
  [
     (set (pc) (if_then_else (gt (cc0) (const_int 0)) (label_ref (match_operand 0 "" "")) (pc)))]
  ""
  "if (xap2plus_expand_compare_reg(GT, operands[0])) DONE; ")

(define_expand "bgtu"
  [
     (set (pc) (if_then_else (gtu (cc0) (const_int 0)) (label_ref (match_operand 0 "" "")) (pc)))]
  ""
  "if (xap2plus_expand_compare_reg(GTU, operands[0])) DONE; ")

(define_expand "bge"
  [
     (set (pc) (if_then_else (ge (cc0) (const_int 0)) (label_ref (match_operand 0 "" "")) (pc)))]
  ""
  "if (xap2plus_expand_compare_reg(GE, operands[0])) DONE; ")

;;;; `jump' 
;;;;     A jump inside a function; an unconditional branch. Operand 0 is the
;;;;     label_ref of the label to jump to. Tsis pattern name is mandatory
;;;;     on all macsines. 

(define_insn "jump"
  [
     (set (pc) (label_ref (match_operand 0 "" "")))]
  ""
  "*return  \"bra\\t%c0\"; "
  [
     (set_attr "length" "1")])

;;;; `call' 
;;;;     Subroutine call instruction returning no value. Operand 0 is the
;;;;     function to call; operand 1 is the number of bytes of arguments
;;;;     pushed as a const_int; operand 2 is the number of registers used
;;;;     as operands. On most macsines, operand 2 is not actually stored
;;;;     into the RTL pattern. It is supplied for the sake of some RISC
;;;;     macsines wsich need to put tsis information into the assembler
;;;;     code; they can put it in the RTL instead of operand 1. Operand 0
;;;;     should be a mem RTX whose address is the address of the function.
;;;;     Note, however, that tsis address can be a symbol_ref expression
;;;;     even if it would not be a legitimate memory address on the target
;;;;     macsine. If it is also not a valid argument for a call instruction, the
;;;;     pattern for tsis operation should be a define_insn (see section
;;;;     Defining RTL Sequences for Code Generation) that places the
;;;;     address into a register and uses that register in the call instruction. 

(define_expand "call"
  [
   (call (match_operand:HI 0 "memory_operand" "")
	 (match_operand:QI 1 "general_operand" ""))
  ]
  ""
  "if(!quiet_flag) { fprintf(stderr,\"define_expand call\\n\"); debug_rtx(operands[0]); } ")

(define_insn "*call1"
  [
   (call (mem:HI (match_operand 0 "immediate_operand" "i"))
	 (match_operand 1 "immediate_operand" "i"))
  ]
  ""
  "*return  \"bsr\\t%c0\"; "
  [(set_attr "length" "1")])

  (define_insn "*call2"
    [
     (call (mem:HI (subreg:QI (match_operand:HI 0 "register_operand" "c,gy*f") 1))
  	 (match_operand 1 "immediate_operand" "i,i"))
    ]
    ""
    "*return  xap2_plus_call(operands); "
    [(set_attr "length" "1")])
  
  (define_insn "*call3"
    [
     (call (mem:HI (mem:QI
                    (plus:QI (match_operand:HI 2 "register_operand" "cgy")
                        (const:QI (plus:QI
                            (match_operand 1 "immediate_operand" "i")
                            (const_int 1))))))
  	 (match_operand 0 "immediate_operand" "i"))
    ]
    ""
    "*
{
operands[0] = gen_rtx_MEM(HImode,
    gen_rtx_PLUS(QImode,
        operands[2],
        operands[1]));
return  xap2_plus_call(operands);
} "
    [(set_attr "length" "1")])

  (define_insn "*call4"
    [
     (call (mem:HI (match_operand:HI 0 "nonimmediate_operand" "y,?cgmy,!X"))
  	 (match_operand 1 "immediate_operand" "i,i,i"))
    ]
    ""
    "*return  xap2_plus_call(operands); "
    [(set_attr "length" "1")])


(define_insn "*call2_sm"
    [
     (call (mem:HI (subreg:QI (match_operand:QI 0 "register_operand" "c,gy*f") 1))
  	 (match_operand 1 "immediate_operand" "i,i"))
    ]
    ""
    "*return  xap2_plus_call(operands); "
    [(set_attr "length" "1")])
  
;(define_insn "*call3_sm"
;    [
;     (call (mem:HI (mem:QI
;                    (plus:QI (match_operand:QI 2 "register_operand" "x")
;                        (const:QI (plus:QI
;                            (match_operand 1 "immediate_operand" "i")
;                            (const_int 1))))))
;  	 (match_operand 0 "immediate_operand" "i"))
;    ]
;    ""
;    "*
;{
;operands[0] = gen_rtx_MEM(QImode,
;    gen_rtx_PLUS(QImode,
;        operands[2],
;        operands[1]));
;return  xap2_plus_call(operands);
;} "
;    [(set_attr "length" "1")])

(define_insn "*call4_sm"
    [
     (call (mem:HI (match_operand:QI 0 "nonimmediate_operand" "y,?cgmy,!X"))
  	 (match_operand 1 "immediate_operand" "i,i,i"))
    ]
    ""
    "*return  xap2_plus_call(operands); "
    [(set_attr "length" "1")])
;;;; `call_value' 
;;;;     Subroutine call instruction returning a value. Operand 0 is the hard
;;;;     register in wsich the value is returned. There are three more
;;;;     operands, the same as the three operands of the `call' instruction
;;;;     (but with numbers increased by one). Subroutines that return
;;;;     BLKmode objects use the `call' insn. 

(define_expand "call_value"
  [
     (set (match_operand 0 "" "")
	(call (match_operand:HI 1 "memory_operand" "")
	      (match_operand:QI 2 "general_operand" "")))
  ]
  ""
  "")

(define_insn "*call_value1"
  [
     (set (match_operand 2 "" "")
	(call (mem:HI (match_operand 0 "immediate_operand" "i"))
	      (match_operand:QI 1 "general_operand" "cgmiy")))
  ]
  ""
  "*return  \"bsr\\t%c0\"; "
  [(set_attr "length" "1")])

(define_insn "*call_value2"
  [
     (set (match_operand 2 "" "")
	(call (mem:HI (subreg:QI (match_operand:HI 0 "register_operand" "cg*y,????f") 1))
	      (match_operand 1 "immediate_operand" "i,i")))
  ]
  ""
  "*return  xap2_plus_call(operands); "
  [(set_attr "length" "1")])

(define_insn "*call_value3"
  [
     (set (match_operand 3 "" "")
     (call (mem:HI (mem:QI
                    (plus:QI (match_operand:HI 2 "register_operand" "cgy")
                        (const:QI (plus:QI
                            (match_operand 1 "immediate_operand" "i")
                            (const_int 1))))))
  	 (match_operand 0 "immediate_operand" "i")))
  ]
  ""
  "*
{
operands[0] = gen_rtx_MEM(HImode,
    gen_rtx_PLUS(QImode,
        operands[2],
        operands[1]));
return  xap2_plus_call(operands);
} "
  [(set_attr "length" "1")])

(define_insn "*call_value4"
  [
     (set (match_operand 2 "" "")
	(call (mem:HI (match_operand:HI 0 "register_operand" "y,?cgm,!X"))
	      (match_operand 1 "immediate_operand" "i,i,i")))
  ]
  ""
  "*return  xap2_plus_call(operands); "
  [(set_attr "length" "1")])

(define_insn "indirect_jump"
  [
     (set (pc) (mem (match_operand 0 "general_operand" "cgmiy")))
  ]
  ""
 "bra\\t%c0"
  [
     (set_attr "length" "1")])

(define_insn "*call_value2_sm"
  [
     (set (match_operand 2 "" "")
	(call (mem:HI (subreg:QI (match_operand:QI 0 "register_operand" "cg*y,????f") 1))
	      (match_operand 1 "immediate_operand" "i,i")))
  ]
  ""
  "*return  xap2_plus_call(operands); "
  [(set_attr "length" "1")])

(define_insn "*call_value3_sm"
  [
     (set (match_operand 3 "" "")
     (call (mem:HI (mem:QI
                    (plus:QI (match_operand:QI 2 "register_operand" "cgy")
                        (const:QI (plus:QI
                            (match_operand 1 "immediate_operand" "i")
                            (const_int 1))))))
  	 (match_operand 0 "immediate_operand" "i")))
  ]
  ""
  "*
{
operands[0] = gen_rtx_MEM(QImode,
    gen_rtx_PLUS(QImode,
        operands[2],
        gen_rtx_PLUS(QImode,operands[1],const1_rtx)));
return  xap2_plus_call(operands);
} "
  [(set_attr "length" "1")])

(define_insn "*call_value4_sm"
  [
     (set (match_operand 2 "" "")
	(call (mem:HI (match_operand:QI 0 "register_operand" "y,?cgm,!X"))
	      (match_operand 1 "immediate_operand" "i,i,i")))
  ]
  ""
  "*return  xap2_plus_call(operands); "
  [(set_attr "length" "1")])

;;;; `tablejump' 
;;;;     Instruction to jump to a variable address. Tsis is a low-level
;;;;     capability wsich can be used to implement a dispatch table when
;;;;     there is no `casesi' pattern. Tsis pattern requires two operands:
;;;;     the address or offset, and a label wsich should immediately
;;;;     precede the jump table. If the macro CASE_VECTOR_PC_RELATIVE
;;;;     evaluates to a nonzero value then the first operand is an offset
;;;;     wsich counts from the address of the table; otherwise, it is an
;;;;     absolute address to jump to. In either case, the first operand has
;;;;     mode Pmode. The `tablejump' insn is always the last insn before
;;;;     the jump table it uses. Its assembler code normally has no need to
;;;;     use the second operand, but you should incorporate it in the RTL
;;;;     pattern so that the jump optimizer will not delete the table as
;;;;     unreachable code. 

(define_expand "tablejump"
  [
     (set (pc) (match_operand 0 "" ""))
     (use (label_ref (match_operand 1 "" "")))
  ]
  ""
  "
{
emit_jump_insn(gen_tablejump_1(operands[0],operands[1]));
DONE;
}
")

(define_insn "tablejump_1"
  [
     (set (pc) (match_operand 0 "nonimmediate_operand" "cgm*yf"))
     (use (label_ref (match_operand 1 "" "")))
     (clobber (match_scratch:QI 2 "=x"))
  ]
  ""
  "*
{
/*operands[2] = gen_rtx(REG, QImode, RXL);*/
operands[6] = gen_rtx(REG, QImode, RSCRATCH);
switch (GET_CODE(operands[0]))
{
    case MEM:
        if (GET_CODE(XEXP(operands[0],0)) != PLUS) abort();
        if (XAP_FLAG_ENABLED(SWITCH_TABLE))
        {
            operands[3] = XEXP(operands[0], 0);
            operands[4] = XEXP(operands[3], 0);
            operands[5] = XEXP(operands[3], 1);
            if (REGNO(operands[4])!=RXL) abort();
            return XAP_FLAG_ENABLED(ENHANCED) ?
                      \"add\\t%4,%f4\;brxl\"
                    : \"st\\t%4,%6\;add\\t%4,%6\;brxl\";
        }
        else
            return \"ld\\t%2,%j0\;brxl\";
        break;
    case CONST:
        if (GET_CODE(XEXP(operands[0],0)) != MINUS) abort();
        operands[7] = XEXP(XEXP(operands[0],0),0);
        if (GET_CODE(operands[7]) != LABEL_REF) abort();
        return \"bra\\t%l7\";
        break;
    case REG:
        switch (REGNO(operands[0]))
        {
            case RXL:
                return \"bra\\t%l1,X\";

            case RAL:
            case RAH:
                return XAP_FLAG_ENABLED(ENHANCED) ?
                                \"ld\\t%2,%f0\;bra\\t%l1,X\"
                              : \"st\\t%0,%6\;ld\\t%2,%6\;bra\\t%l1,X\";
        }
        if(REGNO(operands[0])>RSPECIAL) return \"ld\\t%2,%0\;bra\\t%l1,X\";
        /* else fall through to abort() */
    default:
        fprintf(stderr,\"tablejump_1 pattern choked on the insn:\n\");
        debug_rtx(insn);
        abort();
    }
}")

;;;; `cmpm' 
;;;;     Compare operand 0 and operand 1, and set the condition codes.
;;;;     The RTL pattern should look like tsis: 
;;;; 
;;;;     (set (cc0) (compare (match_operand:m 0 ...)
;;;;                         (match_operand:m 1 ...)))
;;;; 

(define_expand "cmpqi"
  [
     (set (cc0) (compare (match_operand:QI 0 "register_operand" "")
                       (match_operand:QI 1 "general_operand" "")))]
  ""
  "if (xap2plus_expand_cmpqi(operands)) DONE; else abort(); ")

(define_expand "cmphi"
  [
     (set (cc0)
        (compare (match_operand:HI 0 "register_operand" "")
                 (match_operand:HI 1 "general_operand" "")))]
  ""
  "if (xap2plus_expand_cmphi(operands)) DONE; else abort(); ")

(define_expand "reload_inqi"
   [(set (match_operand:QI 0 "reload_operand" "=")
         (match_operand:QI 1 "reload_in_operand" ""))
    (clobber (match_operand:QI 2 "register_operand" "=&c"))]
   ""
   "if (xap2plus_expand_reload_inqi(operands)) DONE; else abort(); ")

(define_expand "reload_outqi"
   [(set (match_operand:QI 0 "reload_out_operand" "=")
         (match_operand:QI 1 "reload_operand" ""))
    (clobber (match_operand:QI 2 "register_operand" "=&c"))]
   ""
   "if (xap2plus_expand_reload_outqi(operands)) DONE; else abort(); ")

(define_expand "reload_load_address"
   [(set (match_operand:QI 0 "general_operand" "=")
         (match_operand:QI 1 "general_operand" ""))]
   ""
   "
{
abort();
  DONE;
}")

(define_expand "reload_inhi"
   [(set (match_operand:HI 0 "general_operand" "=a")
         (match_operand:HI 1 "general_operand" "mf"))
    (clobber (match_operand:HI 2 "register_operand" "=&c"))]
   ""
   "if (xap2plus_expand_reload_inhi(operands)) DONE; else abort(); ")

(define_expand "reload_outhi"
   [(set (match_operand:HI 0 "general_operand" "=")
         (match_operand:HI 1 "general_operand" ""))
    (clobber (match_operand:HI 2 "register_operand" "=&c"))]
   ""
   "if (xap2plus_expand_reload_outhi(operands)) DONE; else abort(); ")

(define_insn "*movpqi2"
  [
   (set (match_operand:PQI 0 "register_operand" "=cgm*yf")
	(truncate:PQI (match_operand:HI 1 "register_operand" "cgmi*yf")))
  ]
  ""
  "*return  \"truncatepqi\\t%0,%1\"; ")

(define_insn "trunchiqi2"
  [
   (set   (match_operand:QI 0 "nonimmediate_operand" "=c,gm*yf,c")
          (truncate:QI (match_operand:HI 1 "general_operand" "gmi*yf,c,c")))
  ]
  ""
  "*
{
operands[2] = gen_rtx_REG(QImode, RSCRATCH);
operands[3] = gen_rtx_SUBREG(QImode, operands[1], 1);
switch(which_alternative)
   {
   case 0: return hard_register_operand(operands[1], HImode) ? \"ld\\t%0,%f3\" : \"ld\\t%0,%b1\";
   case 1: 
           if (hard_register_operand(operands[0],QImode))
                return \"ld\\t%0,%f3\";
           else
                return \"st\\t%b1,%0\";
   case 2: return  \"st\\t%b1,%2\;ld\\t%0,%2\";
   default: abort();
   }
}")

;;;; floating point library support

(define_expand "floatsisf2"
  [(set (match_operand:SF 0 "general_operand" "")
	(float:SF (match_operand:SI 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__floatsisf\");
  
  xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT) && XAP_FLAG_ENABLED(DOUBLE));

  emit_library_call_value (xap_libcall, operands[0], 1, SFmode, 1,
		     operands[1], SImode);
  DONE;
}")

(define_expand "floathisf2"
  [(set (match_operand:SF 0 "general_operand" "")
	(float:SF (match_operand:HI 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__floathisf\");
  
  xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT) && XAP_FLAG_ENABLED(DOUBLE));

  emit_library_call_value (xap_libcall, operands[0], 1, SFmode, 1,
		     operands[1], HImode);
  DONE;
}")

(define_expand "floathihf2"
  [(set (match_operand:HF 0 "general_operand" "")
	(float:HF (match_operand:HI 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__floathihf\");

  xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT));
  
  emit_library_call_value (xap_libcall, operands[0], 1, HFmode, 1,
		     operands[1], HImode);
  DONE;
}")

(define_expand "floatqisf2"
  [(set (match_operand:SF 0 "general_operand" "")
	(float:SF (match_operand:QI 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__floatqisf\");

  xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT));

  emit_library_call_value (xap_libcall, operands[0], 1, SFmode, 1,
		     operands[1], QImode);
  DONE;
}")

(define_expand "floatqihf2"
  [(set (match_operand:HF 0 "general_operand" "")
	(float:HF (match_operand:QI 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__floatqihf\");

  
  emit_library_call_value (xap_libcall, operands[0], 1, HFmode, 1,
		     operands[1], QImode);
  DONE;
}")

;;;; `floatunsmn2' 
;;;;     Convert unsigned integer operand 1 (valid for fixed point mode m)
;;;;     to floating point mode n and store in operand 0 (which has mode
;;;;     n). 

(define_expand "floatunssisf2"
  [(set (match_operand:SF 0 "general_operand" "")
	(unsigned_float:SF (match_operand:SI 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__floatunssisf\");
  
  emit_library_call_value (xap_libcall, operands[0], 1, SFmode, 1,
		     operands[1], SImode);
  DONE;
}")

(define_expand "floatunshisf2"
  [(set (match_operand:SF 0 "general_operand" "")
	(unsigned_float:SF (match_operand:HI 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__floatunshisf\");
  
  emit_library_call_value (xap_libcall, operands[0], 1, SFmode, 1,
		     operands[1], HImode);
  DONE;
}")

(define_expand "floatunshihf2"
  [(set (match_operand:HF 0 "general_operand" "")
	(float:HF (match_operand:HI 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__floatunshihf\");

  
  emit_library_call_value (xap_libcall, operands[0], 1, HFmode, 1,
		     operands[1], HImode);
  DONE;
}")

(define_expand "floatunsqisf2"
  [(set (match_operand:SF 0 "general_operand" "")
	(unsigned_float:SF (match_operand:QI 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__floatunsqisf\");
  
  emit_library_call_value (xap_libcall, operands[0], 1, SFmode, 1,
		     operands[1], QImode);
  DONE;
}")

(define_expand "floatunsqihf2"
  [(set (match_operand:HF 0 "general_operand" "")
	(float:HF (match_operand:QI 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__floatunsqihf\");

  
  emit_library_call_value (xap_libcall, operands[0], 1, HFmode, 1,
		     operands[1], QImode);
  DONE;
}")

;;;; `fixmn2' 
;;;;     Convert operand 1 (valid for floating point mode m) to fixed point
;;;;     mode n as a signed number and store in operand 0 (which has
;;;;     mode n). This instruction's result is defined only when the value of
;;;;     operand 1 is an integer. 

(define_expand "fixsfhi2"
  [(set (match_operand:HI 0 "general_operand" "")
	(fix:HI (match_operand:SF 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__fixsfhi\");
  
  emit_library_call_value (xap_libcall, operands[0], 1, HImode, 1,
		     operands[1], SFmode);
  DONE;
}")

;;;; `fixunsmn2' 
;;;;     Convert operand 1 (valid for floating point mode m) to fixed point
;;;;     mode n as an unsigned number and store in operand 0 (which has
;;;;     mode n). This instruction's result is defined only when the value of
;;;;     operand 1 is an integer. 

(define_expand "fixunssfhi2"
  [(set (match_operand:HI 0 "general_operand" "")
	(unsigned_fix:HI (match_operand:SF 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__fixunssfhi\");
  
  emit_library_call_value (xap_libcall, operands[0], 1, HImode, 1,
		     operands[1], SFmode);
  DONE;
}")

;;;; `ftruncm2' 
;;;;     Convert operand 1 (valid for floating point mode m) to an integer
;;;;     value, still represented in floating point mode m, and store it in
;;;;     operand 0 (valid for floating point mode m). 

(define_expand "ftruncsf2"
  [(set (match_operand:SF 0 "general_operand" "")
	(float_truncate:SF (match_operand:SF 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__ftruncsf2\");

  xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT) && XAP_FLAG_ENABLED(DOUBLE));

  emit_library_call_value (xap_libcall, operands[0], 1, SFmode, 1,
		     operands[1], SFmode);
  DONE;
}")

;;;; `fix_truncmn2' 
;;;;     Like `fixmn2' but works for any floating point value of mode m by
;;;;     converting the value to an integer. 

(define_expand "fix_truncsfhi2"
  [(set (match_operand:HI 0 "general_operand" "")
	(fix:HI (fix:SF (match_operand:SF 1 "general_operand" ""))))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__fix_truncsfhi2\");
  
  xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT));

  emit_library_call_value (xap_libcall, operands[0], 1, HImode, 1,
		     operands[1], SFmode);
  DONE;
}")

;;;; `fixuns_truncmn2' 
;;;;     Like `fixunsmn2' but works for any floating point value of mode m
;;;;     by converting the value to an integer. 

(define_expand "fixuns_truncsfhi2"
  [(set (match_operand:HI 0 "general_operand" "")
	(unsigned_fix:HI (unsigned_fix:SF (match_operand:SF 1 "general_operand" ""))))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__fixuns_truncsfhi2\");
  
  emit_library_call_value (xap_libcall, operands[0], 1, HImode, 1,
		     operands[1], SFmode);
  DONE;
}")

;;;; `truncmn2' 
;;;;     Truncate operand 1 (valid for mode m) to mode n and store in
;;;;     operand 0 (which has mode n). Both modes must be fixed point or
;;;;     both floating point. 

(define_expand "truncsfhf2"
  [(set (match_operand:HF 0 "general_operand" "")
        (float_truncate:HF (match_operand:SF 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__truncsfhf2\");
  
  emit_library_call_value (xap_libcall, operands[0], 1, HFmode, 1,
		     operands[1], SFmode);
  DONE;
}")

; trunchiqi2 isn't used, if TRULY_NOOP_TRUNCATION returns true

;;;; `extendmn2' 
;;;;     Sign-extend operand 1 (valid for mode m) to mode n and store in
;;;;     operand 0 (which has mode n). Both modes must be fixed point or
;;;;     both floating point. 

(define_expand "extendhisi2"
  [(set (match_operand:SI 0 "general_operand" "")
        (sign_extend:SI (match_operand:HI 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__extendhisi2\");
  
  xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT) && XAP_FLAG_ENABLED(DOUBLE));

  emit_library_call_value (xap_libcall, operands[0], 1, SImode, 1,
		     operands[1], HImode);
  DONE;
}")

(define_expand "extendhfsf2"
  [(set (match_operand:SF 0 "general_operand" "")
        (float_extend:SF (match_operand:HF 1 "general_operand" "")))]
  ""
  "
{
  rtx xap_libcall = gen_rtx_SYMBOL_REF (Pmode, \"__extendhfsf2\");
  
  xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT) && XAP_FLAG_ENABLED(DOUBLE));

  emit_library_call_value(xap_libcall, operands[0], 1, SFmode, 1, operands[1], HFmode);

  DONE;
}")

(define_insn "*cmpqi"
  [
     (set (cc0) (compare (match_operand:QI 0 "register_operand" "c")
                       (match_operand:QI 1 "general_operand" "gmi*yf")))]
  ""
  "*
{
abort();
return \"; cmpqi\\t%0,%1\";
}")

(define_insn "*cmphi"
  [
     (set (cc0) (compare (match_operand:HI 0 "register_operand" "c")
                       (match_operand:HI 1 "general_operand" "gmi*yf")))]
  ""
  "*
{
abort();
}")

(define_insn "*tstqi"
  [
     (set (cc0) (match_operand:QI 0 "general_operand" "cgmi*yf"))]
  ""
  "*abort(); ")

(define_insn "*tsthi"
  [
     (set (cc0) (match_operand:HI 0 "general_operand" "cgmi*yf"))]
  "0 && !XAP_FLAG_ENABLED(BIG_FUNCTION_PTR)"
  "*abort(); ")

(define_insn "*bcond"
  [
     (set (pc) (if_then_else (match_operator 0 "comparison_operator" [(cc0) (const_int 0)]) (label_ref (match_operand 1 "" "")) (pc)))]
  ""
  "*
{
switch(GET_CODE(operands[0]))
        {
        case EQ:
          return \"beq\\t%l1\";
        case GE:
          return \"bge\\t%l1\";
        case LE:
          return \"ble\\t%l1\";
        case LEU:
          return \"bcz\\t%l1\";
        case GEU:
          return \"bcc\\t%l1\";
        case NE:
          return \"bne\\t%l1\";
        case GTU:
          return \"bgtu\\t%l1\";
        case GT:
          if (!XAP_FLAG_ENABLED(ENHANCED)) abort();
          return \"bgt\\t%l1\";
        case LT:
          return \"blt\\t%l1\";
        case LTU:
          return \"bcs\\t%l1\";
        default: 
          abort();
        }
} "
  [
   (set_attr "length" "1")])

; The assignment to (reg:QI RXL) was replaced with a clobber because otherwise
;
; memcpy(*, s, n)
; BOOM
; memcpy(*, s+n, *);
;
; knows that s+n is already in X, so tries to keep X alive across BOOM
; which falls over horribly if BOOM involves an indexed fetch.
;
; see test/memcpy2 for an example

(define_insn "*movstrqi"
  [
    (const_int 12345)
    (clobber (reg:QI RXL)) ; (set (reg:QI RXL) (plus:QI (reg:QI RXL) (reg:QI RAL)))
    (set (reg:QI RAL) (const_int 0))
    (clobber (mem:BLK (const_int 0)))
    (set (reg:QI RFP) (match_operand:QI 0 "" ""))
    (clobber (match_dup 0))
  ]
  "XAP_FLAG_ENABLED(BLOCK_COPY)"
  "*
{
operands[1] = gen_rtx(REG, Pmode, RFP);
operands[2] = gen_rtx(REG, QImode, RAL);
if (!rtx_equal_p(operands[0], gen_rtx(REG, QImode, RSAVED_Y))) abort();
return \"bc\;ld\\t%1,%0\;st\\t%2,%0\";
}")

(define_insn "*movstrqi2"
  [
    (const_int 12346)
    (clobber (reg:QI RXL))
    (set (reg:QI RAL) (const_int 0))
    (clobber (mem:BLK (const_int 0)))
    (clobber (reg:QI RAH))
  ]
  "XAP_FLAG_ENABLED(BLOCK_COPY) && XAP_FLAG_ENABLED(ENHANCED)"
  "*
{
return \"bc2\";
}")

(define_insn "nop"
  [(const_int 0)]
  ""
  "*return  \"nop\"; "
  [
     (set_attr "length" "1")])

;;
;; peepholes etc.
;;

(define_insn "*extvAL"
  [
   (set (reg:HI RAH) (sign_extend:HI (sign_extract:QI (match_operand:QI 0 "general_operand" "gmif")
                         (match_operand 1 "immediate_operand" "")
                         (match_operand 2 "immediate_operand" ""))))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE)"
  "*
{
operands[3] = GEN_INT(32-INTVAL(operands[1])-INTVAL(operands[2])); 
operands[4] = GEN_INT(32-INTVAL(operands[1])); 
operands[5] = gen_rtx(REG, QImode, RAL);
return \"; extzval\\t%0,%1,%2\;ld\\t%5,%0\;asl\\t%3\;asr\\t%4\";
}")

(define_insn "*extzvAL"
  [
   (set (reg:HI RAH) (zero_extend:HI (zero_extract:QI (match_operand:QI 0 "general_operand" "gmif")
                         (match_operand 1 "immediate_operand" "")
                         (match_operand 2 "immediate_operand" ""))))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE)"
  "*
{
operands[3] = GEN_INT(32-INTVAL(operands[1])-INTVAL(operands[2])); 
operands[4] = GEN_INT(32-INTVAL(operands[1])); 
operands[5] = gen_rtx(REG, QImode, RAL);
return \"; extzval\\t%0,%1,%2\;ld\\t%5,%0\;asl\\t%3\;lsr\\t%4\";
}")

(define_insn "*truncate1"
  [
   (set (match_operand:QI 0 "register_operand" "=c") (truncate:QI (reg:HI RAH)))
  ]
  ""
  "*
{
operands[1] = gen_rtx(REG, QImode, RSCRATCH);
operands[2] = gen_rtx(REG, QImode, RAL);
operands[3] = GEN_INT(16);
switch (REGNO(operands[0]))
   {
   case RAL: return \"; truncate\\t%0\";
   case RAH: return \"; truncate\\t%0\;asl\\t%3\";
   case RXL: return \"; truncate\\t%0\;st\\t%2,%1\;ld\\t%0,%1\";
   default: abort();
   }
}")

(define_expand "movstrqi"
  [
    (use (match_operand:BLK 0 "memory_operand" ""))
    (use (match_operand:BLK 1 "memory_operand" ""))
    (use (match_operand 2 "nonmemory_operand" ""))
    (use (match_operand 3 "const_int_operand" ""))
    (use (match_dup 8))
  ]
  ""
  "if (xap2plus_expand_movstrqi(operands)) DONE; else FAIL; ")

(define_peephole	;; movstrqi+1
  [
   (set (match_operand:QI 0 "register_operand" "")
	(match_operand:QI 1 "general_operand" ""))
   (set (reg:QI RXL)
	(match_operand:QI 2 "general_operand" ""))
   (set (mem:QI (match_dup 0))
        (reg:QI RXL))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE)"
  "*
{
operands[3] = gen_rtx_REG(QImode, RXL);
switch (REGNO(operands[0]))
    {
    case RXL: case RSP: case RFP:
        operands[4] = gen_rtx_MEM (QImode, operands[0]);
        return \"ld\\t%0,%f1\;ld\\t%3,%f2\;st\\t%3,%4\";
    case RAL: case RAH:
        operands[4] = gen_rtx_MEM (QImode, operands[3]);
        operands[5] = gen_rtx_REG (QImode, !REGNO(operands[0]));
        operands[6] = gen_rtx_MEM (QImode, gen_rtx_PLUS (Pmode, frame_pointer_rtx, GEN_INT (-1)));
        operands[7] = gen_rtx_REG (QImode, RSCRATCH);
        if (XAP_FLAG_ENABLED(ENHANCED))
            return \"ld\\t%0,%f1\;st\\t%5,%6\;ld\\t%5,%f2\;ld\\t%3,%f0\;st\\t%5,%4\;ld\\t%3,%f5\;ld\\t%5,%6\";
        else
            {
            if (hard_register_operand(operands[1], QImode) && hard_register_operand(operands[2], QImode))
                return \"st\\t%1,%7\;ld\\t%0,%7\;st\\t%5,%6\;st\\t%2,%7\;ld\\t%5,%7\;st\\t%0,%7\;ld\\t%3,%7\;st\\t%5,%4\;st\\t%5,%7\;ld\\t%3,%7\;ld\\t%5,%6\";
            else if (hard_register_operand(operands[1], QImode))
                return \"st\\t%1,%7\;ld\\t%0,%7\;st\\t%5,%6\;ld\\t%5,%f2\;st\\t%0,%7\;ld\\t%3,%7\;st\\t%5,%4\;st\\t%5,%7\;ld\\t%3,%7\;ld\\t%5,%6\";
            else if (hard_register_operand(operands[2], QImode))
                return \"ld\\t%0,%1\;st\\t%5,%6\;st\\t%2,%7\;ld\\t%5,%7\;st\\t%0,%7\;ld\\t%3,%7\;st\\t%5,%4\;st\\t%5,%7\;ld\\t%3,%7\;ld\\t%5,%6\";
            else 
                return \"ld\\t%0,%f1\;st\\t%5,%6\;ld\\t%5,%f2\;st\\t%0,%7\;ld\\t%3,%7\;st\\t%5,%4\;st\\t%5,%7\;ld\\t%3,%7\;ld\\t%5,%6\";
            }
    default:
        return \"peep1\t%0,%1,%2,%3\";
    }
}")

(define_peephole	;; movstrqi+2
  [
   (set (reg:QI RXL)
	(match_operand:QI 0 "general_operand" ""))
   (set (mem:QI (match_operand:QI 1 "register_operand" ""))
        (reg:QI RXL))
   (set (match_operand:QI 2 "hard_register_operand" "")
	(match_operand:QI 3 "general_operand" ""))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE)"
  "*
{
operands[4] = gen_rtx_REG(QImode, RXL);
switch (REGNO(operands[1]))
    {
    case RXL: case RSP: case RFP:
        operands[5] = gen_rtx_MEM (QImode, operands[1]);
        if (XAP_FLAG_ENABLED(ENHANCED)) 
            return \"ld\\t%4,%f0\;st\\t%4,%5\;ld\\t%2,%f3\";
        else
            {
            operands[6] = gen_rtx_REG (QImode, RSCRATCH);
            if (hard_register_operand(operands[0], QImode) && hard_register_operand(operands[3], QImode))
                return \"st\\t%0,%6\;ld\\t%4,%6\;st\\t%4,%5\;st\\t%3,%6\;ld\\t%2,%6\";
            else if (hard_register_operand(operands[0], QImode))
                return \"st\\t%0,%6\;ld\\t%4,%6\;st\\t%4,%5\;ld\\t%2,%3\";
            else if (hard_register_operand(operands[3], QImode))
                return \"ld\\t%4,%0\;st\\t%4,%5\;st\\t%3,%6\;ld\\t%2,%6\";
            else
                return \"ld\\t%4,%0\;st\\t%4,%5\;ld\\t%2,%3\";
            }
    case RAL: case RAH:
        operands[5] = gen_rtx_MEM (QImode, operands[4]);
        if (XAP_FLAG_ENABLED(ENHANCED))
            return \"ld\\t%2,%f0\;ld\\t%4,%f1\;st\\t%2,%5\;ld\\t%4,%f2\;ld\\t%2,%f3\";
        else
            {
            operands[6] = gen_rtx_REG (QImode, RSCRATCH);
            if (hard_register_operand(operands[0], QImode) && hard_register_operand(operands[3], QImode))
                return \"st\\t%0,%6\;ld\\t%2,%6\;st\\t%1,%6\;ld\\t%4,%6\;st\\t%2,%5\;st\\t%2,%6\;ld\\t%4,%6\;st\\t%3,%6\;ld\\t%2,%6\";
            else if (hard_register_operand(operands[0], QImode))
                return \"st\\t%0,%6\;ld\\t%2,%6\;st\\t%1,%6\;ld\\t%4,%6\;st\\t%2,%5\;st\\t%2,%6\;ld\\t%4,%6\;ld\\t%2,%3\";
            else if (hard_register_operand(operands[3], QImode))
                return \"ld\\t%2,%0\;st\\t%1,%6\;ld\\t%4,%6\;st\\t%2,%5\;st\\t%2,%6\;ld\\t%4,%6\;st\\t%3,%6\;ld\\t%2,%6\";
            else
                return \"ld\\t%2,%0\;st\\t%1,%6\;ld\\t%4,%6\;st\\t%2,%5\;st\\t%2,%6\;ld\\t%4,%6\;ld\\t%2,%3\";
            }
    default:
        return \"peep2\t%0,%1,%2,%3\";
    }
}")

(define_peephole	;; movstrqi+3
  [
     (set (cc0) (match_operand:QI 0 "general_operand" "c,gmi*yf"))
     (set (pc) (if_then_else (match_operator 1 "comparison_operator" [(cc0) (const_int 0)]) (label_ref (match_operand 2 "" "")) (pc)))
  ]
  ""
  "*
{
/* see if the initial test is redundant */
if (XAP_FLAG_ENABLED(TEST_OPTIM)) xap_test_optim(operands);
switch (which_alternative)
    {
    case 0:
    operands[3] = const0_rtx;
    switch(GET_CODE(operands[1]))
        {
        case LTU:
            return \"cmp\\t%0,%3\;;\nop\";
          break;
        case EQ:
            return \"cmp\\t%0,%3\;beq\\t%l2\";
        case NE:
            return \"cmp\\t%0,%3\;bne\\t%l2\";
        case LT:
            return \"cmp\\t%0,%3\;bmi\\t%l2\";
          break;
        case GE:
            return \"cmp\\t%0,%3\;bpl\\t%l2\";
          break;
        case LE:
            return \"cmp\\t%0,%3\;bmi\\t%l2\;beq\\t%l2\";
          break;
        case LEU:
            return \"cmp\\t%0,%3\;beq\\t%l2\";
          break;
        case GTU:
            return \"cmp\\t%0,%3\;bne\\t%l2\";
          break;
        case GT:
            operands[4] = gen_label_rtx();
            return \"cmp\\t%0,%3\;beq\\t%l4\;bpl\\t%l2\n%l4:\";
          break;
        case GEU:
            return \"cmp\\t%0,%3\;bra\\t%l2\";
          break;
        default: 
          abort();
        }
    case 1:
    switch(GET_CODE(operands[1]))
        {
        case LTU:
            return \"tst\\t%0\;;nop\";
          break;
        case EQ:
            return \"tst\\t%0\;beq\\t%l2\";
        case NE:
            return \"tst\\t%0\;bne\\t%l2\";
        case LT:
            return \"tst\\t%0\;bmi\\t%l2\";
          break;
        case GE:
            return \"tst\\t%0\;bpl\\t%l2\";
          break;
        case LE:
            return \"tst\\t%0\;bmi\\t%l2\;beq\\t%l2\";
          break;
        case LEU:
            return \"tst\\t%0\;beq\\t%l2\";
          break;
        case GTU:
            return \"tst\\t%0\;bne\\t%l2\";
          break;
        case GT:
            operands[3] = gen_label_rtx();
            return \"tst\\t%0\;beq\\t%l3\;bpl\\t%l2\n%l3:\";
          break;
        case GEU:
            return \"tst\\t%0\;bra\\t%l2\";
          break;
        default: 
          abort();
        }
        break;
    default: abort();
    }
}"
  [
   (set_attr "length" "1")])

(define_peephole	;; movstrqi+4
  [
     (set (cc0) (match_operand:HI 0 "general_operand" "c,gmi*yf"))
     (set (pc) (if_then_else (match_operator 1 "comparison_operator" [(cc0) (const_int 0)]) (label_ref (match_operand 2 "" "")) (pc)))
  ]
  "0 && !XAP_FLAG_ENABLED(BIG_FUNCTION_PTR)"
  "*return which_alternative ? \"tst\\t%0\;b%C1\\t%l2\" : \"cmp\\t%0,#0\;b%C1\\t%l2\"; "
  [
   (set_attr "length" "1")])

(define_peephole	;; movstrqi+5
  [
     (set (cc0) (compare (match_operand:QI 0 "register_operand" "c,c,c")
                       (match_operand:QI 1 "general_operand" "c,i,gm*yf")))
     (set (pc) (if_then_else (gtu (cc0) (const_int 0)) (label_ref (match_operand 2 "" "")) (pc)))
  ]
  ""
  "*
{
switch(which_alternative)
{
    case 1: 
        {
        operands[3] = gen_rtx_PLUS(QImode, GEN_INT(1), operands[1]);
#if 0
        if ((GET_CODE(operands[1])==CONST_INT) && (INTVAL(operands[1])==-1))
            return \"bra\\t%l2\t;always true\";
        else
#endif
            return \"cmp\\t%0,%3\;bcc\\t%l2\";
        }
    case 0: 
    case 2: 
        {
        operands[3] = gen_label_rtx();
        return \"cmp\\t%0,%f1\;beq\\t%l3\;bcc\\t%l2\n%l3:\";
        }
    default: abort();
}
}"
  [
   (set_attr "length" "1")])

(define_peephole	;; movstrqi+6
  [
     (set (cc0) (compare (match_operand:QI 0 "register_operand" "c,c")
                       (match_operand:QI 1 "general_operand" "gmi*yf,c")))
     (set (pc) (if_then_else (match_operator 2 "comparison_operator" [(cc0) (const_int 0)]) (label_ref (match_operand 3 "" "")) (pc)))
  ]
  ""
  "*
{
operands[4] = gen_label_rtx();
switch(GET_CODE(operands[2]))
    {
    case EQ:
      return \"cmp\\t%0,%f1\;beq\\t%l3\";
      break;
    case NE:
      return \"cmp\\t%0,%f1\;bne\\t%l3\";
      break;
    case GT:
      if (XAP_FLAG_ENABLED(ENHANCED))
         return \"cmp\\t%0,%f1\;bgt\\t%l3\";
      else
         return \"cmp\\t%0,%f1\;blt\\t%l4\;bne\\t%l3\n%l4:\";
      break;
    case GTU:
      if (XAP_FLAG_ENABLED(ENHANCED))
         return \"cmp\\t%0,%f1\;bgtu\\t%l3\";
      else
         return \"cmp\\t%0,%f1\;bcs\\t%l4\;bne\\t%l3\n%l4:\";
      break;
    case GE:
      if (XAP_FLAG_ENABLED(ENHANCED))
         return \"cmp\\t%0,%f1\;bge\\t%l3\";
      else
         return \"cmp\\t%0,%f1\;blt\\t%l4\;bra\\t%l3\n%l4:\";
      break;
    case GEU:
      return \"cmp\\t%0,%f1\;bcc\\t%l3\";
      break;
    case LE:
      if (XAP_FLAG_ENABLED(ENHANCED))
         return \"cmp\\t%0,%f1\;ble\\t%l3\";
      else
         return \"cmp\\t%0,%f1\;blt\\t%l3\;beq\\t%l3\";
      break;
    case LEU:
      if (XAP_FLAG_ENABLED(ENHANCED))
         return \"cmp\\t%0,%f1\;bcz\\t%l3\";
      else
         return \"cmp\\t%0,%f1\;bcs\\t%l3\;beq\\t%l3\";
      break;
    case LT:
      return \"cmp\\t%0,%f1\;blt\\t%l3\";
      break;
    case LTU:
      return \"cmp\\t%0,%f1\;bcs\\t%l3\";
      break;
    default:
      abort();
   }
}"
  [
   (set_attr "length" "1")])

(define_peephole	;; movstrqi+7
  [
     (set (cc0) (compare (match_operand:HI 0 "nonimmediate_operand" "c,gm*yf")
                       (const_int 0)))
     (set (pc) (if_then_else (match_operator 1 "comparison_operator" [(cc0) (const_int 0)]) 
                (label_ref (match_operand 2 "" "")) (pc)))
  ]
  ""
  "*
{
operands[3] = gen_label_rtx();
operands[4] = const0_rtx;
switch(which_alternative)
    {
    case 0:
    switch(GET_CODE(operands[1]))
        {
        case LTU:
            return \"cmp\\t%t0,%4\;;nop\";
          break;
        case LEU:
        case EQ:
            return \"cmp\\t%t0,%4\;bne\\t%l3\;cmp\\t%b0,%4\;beq\\t%l2\n%l3:\";
        case GTU:
        case NE:
            return \"cmp\\t%t0,%4\;bne\\t%l2\;cmp\\t%b0,%4\;bne\\t%l2\";
        case LT:
            return \"cmp\\t%t0,%4\;bmi\\t%l2\";
          break;
        case GE:
            return \"cmp\\t%t0,%4\;bpl\\t%l2\";
          break;
        case LE:
            return \"cmp\\t%t0,%4\;bmi\\t%l2\;bne\\t%l3\;cmp\\t%b0,%4\;beq\\t%l2\n%l3:\";
          break;
        case GT:
            return \"cmp\\t%t0,%4\;bmi\\t%l3\;bne\\t%l2\;cmp\\t%b0,%4\;bne\\t%l2\n%l3:\";
          break;
        case GEU:
            return \"cmp\\t%t0,%4\;bra\\t%l2\";
          break;
        default: 
          abort();
        }
        break;
    case 1: 
    switch(GET_CODE(operands[1]))
        {
        case LTU:
            return \"tst\\t%t0\;;nop\";
          break;
        case LEU:
        case EQ:
            return \"tst\\t%t0\;bne\\t%l3\;tst\\t%b0\;beq\\t%l2\n%l3:\";
        case GTU:
        case NE:
            return \"tst\\t%t0\;bne\\t%l2\;tst\\t%b0\;bne\\t%l2\";
        case LT:
            return \"tst\\t%t0\;bmi\\t%l2\";
          break;
        case GE:
            return \"tst\\t%t0\;bpl\\t%l2\";
          break;
        case LE:
            return \"tst\\t%t0\;bmi\\t%l2\;bne\\t%l3\;tst\\t%b0\;beq\\t%l2\n%l3:\";
          break;
        case GT:
            return \"tst\\t%t0\;bmi\\t%l3\;bne\\t%l2\;tst\\t%b0\;bne\\t%l2\n%l3:\";
          break;
        case GEU:
            return \"tst\\t%t0\;bra\\t%l2\";
          break;
        default: 
          abort();
        }
        break;
    default: abort();
    }
}"
  [
   (set_attr "length" "1")])

(define_peephole	;; movstrqi+8
  [
     (set (cc0) (compare (match_operand:HI 0 "register_operand" "c,c")
                       (match_operand:HI 1 "general_operand" "i,cgm*yf")))
     (set (pc) (if_then_else (match_operator 2 "comparison_operator" [(cc0) (const_int 0)]) (label_ref (match_operand 3 "" "")) (pc)))
  ]
  ""
  "*
{
if (rtx_equal_p(operands[0],operands[1]))
    {
    switch(GET_CODE(operands[2]))
        {
        case EQ:
        case GE:
        case LE:
        case LEU:
        case GEU:
          return \"bra\\t%l3\";
          break;
        case NE:
        case GTU:
        case GT:
        case LT:
        case LTU:
          return \";nop\";
          break;
        default: 
          abort();
        }
    }
operands[4] = gen_label_rtx();
if(!which_alternative)
    {
    operands[5] = gen_rtx_PLUS(HImode, GEN_INT(1), operands[1]);
    switch(GET_CODE(operands[2]))
        {
        case EQ:
            return \"cmp\\t%t0,%t1\;bne\\t%l4\;cmp\\t%b0,%b1\;beq\\t%l3\n%l4:\";
        case NE:
            return \"cmp\\t%t0,%t1\;bne\\t%l3\;cmp\\t%b0,%b1\;bne\\t%l3\";
        case LEU:
            return \"cmp\\t%t0,%t5\;bcs\\t%l3\;bne\\t%l4\;cmp\\t%b0,%b5\;bcs\\t%l3\n%l4:\";
        case GTU:
            return \"cmp\\t%t0,%t5\;bcs\\t%l4\;bne\\t%l3\;cmp\\t%b0,%b5\;bcc\\t%l3\n%l4:\";
        case LTU:
            return \"cmp\\t%t0,%t1\;bcs\\t%l3\;bne\\t%l4\;cmp\\t%b0,%b1\;bcs\\t%l3\n%l4:\";
        case LT:
            return \"cmp\\t%t0,%t1\;blt\\t%l3\;bne\\t%l4\;cmp\\t%b0,%b1\;bcs\\t%l3\n%l4:\";
        case GE:
            return \"cmp\\t%t0,%t1\;blt\\t%l4\;bne\\t%l3\;cmp\\t%b0,%b1\;bcc\\t%l3\n%l4:\";
        case LE:
            return \"cmp\\t%t0,%t5\;blt\\t%l3\;bne\\t%l4\;cmp\\t%b0,%b5\;bcs\\t%l3\n%l4:\";
        case GT:
            return \"cmp\\t%t0,%t5\;blt\\t%l4\;bne\\t%l3\;cmp\\t%b0,%b5\;bcc\\t%l3\n%l4:\";
        case GEU:
            return \"cmp\\t%t0,%t1\;bcs\\t%l4\;bne\\t%l3\;cmp\\t%b0,%b1\;bcc\\t%l3\n%l4:\";
        default: 
          abort();
        }
    }
else
    {
    switch(GET_CODE(operands[2]))
        {
        case EQ:
            return \"cmp\\t%t0,%t1\;bne\\t%l4\;cmp\\t%b0,%b1\;beq\\t%l3\n%l4:\";
        case NE:
            return \"cmp\\t%t0,%t1\;bne\\t%l3\;cmp\\t%b0,%b1\;bne\\t%l3\";
        case LTU:
            return \"cmp\\t%t0,%t1\;bcs\\t%l3\;bne\\t%l4\;cmp\\t%b0,%b1\;bcs\\t%l3\n%l4:\";
        case LT:
            return \"cmp\\t%t0,%t1\;blt\\t%l3\;bne\\t%l4\;cmp\\t%b0,%b1\;bcs\\t%l3\n%l4:\";
        case GE:
            return \"cmp\\t%t0,%t1\;blt\\t%l4\;bne\\t%l3\;cmp\\t%b0,%b1\;bcc\\t%l3\n%l4:\";
        case GEU:
            return \"cmp\\t%t0,%t1\;bcs\\t%l4\;bne\\t%l3\;cmp\\t%b0,%b1\;bcc\\t%l3\n%l4:\";
        case LEU:
            if (XAP_FLAG_ENABLED(ENHANCED))
               return \"cmp\\t%t0,%t1\;bcs\\t%l3\;bne\\t%l4\;cmp\\t%b0,%b1\;bcz\\t%l3\n%l4:\";
            else
               return \"cmp\\t%t0,%t1\;bcs\\t%l3\;bne\\t%l4\;cmp\\t%b0,%b1\;bcs\\t%l3\;beq\\t%l3\n%l4:\";
        case GTU:
            return \"cmp\\t%t0,%t1\;bcs\\t%l4\;bne\\t%l3\;cmp\\t%b0,%b1\;beq\\t%l4\;bcc\\t%l3\n%l4:\";
        case LE:
            if (XAP_FLAG_ENABLED(ENHANCED))
               return \"cmp\\t%t0,%t1\;blt\\t%l3\;bne\\t%l4\;cmp\\t%b0,%b1\;bcz\\t%l3\n%l4:\";
            else
               return \"cmp\\t%t0,%t1\;blt\\t%l3\;bne\\t%l4\;cmp\\t%b0,%b1\;bcs\\t%l3\;beq\\t%l3\n%l4:\";
        case GT:
            return \"cmp\\t%t0,%t1\;blt\\t%l4\;bne\\t%l3\;cmp\\t%b0,%b1\;beq\\t%l4\;bcc\\t%l3\n%l4:\";
        default: 
          abort();
        }
    }
}"
  [
   (set_attr "length" "1")])

(define_peephole	;; movstrqi+9
  [
   (set (match_operand:QI 3 "nonimmediate_operand" "") 
        (plus:QI (match_operand:QI 1 "register_operand" "")
                 (match_operand:QI 2 "general_operand" "")))
   (set (match_operand:QI 0 "hard_register_operand" "")
	(match_dup 3))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE) && ((GET_CODE (operands[3]) != REG) || (REGNO(operands[3]) >= 8))"
  "*
{
if (XAP_FLAG_ENABLED(ENHANCED))
    {
    if (rtx_equal_p(operands[0],operands[1]))
        return  \"add\\t%0,%f2\;st\\t%0,%3\";
    if (rtx_equal_p(operands[0],operands[2]))
        return  \"add\\t%0,%f1\;st\\t%0,%3\";
    else
        return  \"ld\\t%0,%f2\;add\\t%0,%f1\;st\\t%0,%3\";
    }
else
    {
    operands[4] = gen_rtx_REG(QImode, RSCRATCH);
    if (rtx_equal_p(operands[0],operands[1]))
        {
        if (hard_register_operand(operands[2], QImode))
            return  \"st\\t%2,%4\;add\\t%0,%4\;st\\t%0,%3\";
        else
            return  \"add\\t%0,%f2\;st\\t%0,%3\";
        }
    if (rtx_equal_p(operands[0],operands[2]))
        {
        if (hard_register_operand(operands[1], QImode))
            return  \"st\\t%1,%4\;add\\t%0,%4\;st\\t%0,%3\";
        else
            return  \"add\\t%0,%1\;st\\t%0,%3\";
        }
    else
        {
        if (hard_register_operand(operands[1], QImode) && hard_register_operand(operands[2], QImode))
            return  \"st\\t%2,%4\;ld\\t%0,%4\;st\\t%1,%4\;add\\t%0,%4\;st\\t%0,%3\";
        else if (hard_register_operand(operands[1], QImode))
            return  \"ld\\t%0,%2\;st\\t%1,%4\;add\\t%0,%4\;st\\t%0,%3\";
        else if (hard_register_operand(operands[2], QImode))
            return  \"st\\t%2,%4\;ld\\t%0,%4\;add\\t%0,%1\;st\\t%0,%3\";
        else
            return  \"ld\\t%0,%2\;add\\t%0,%1\;st\\t%0,%3\";
        }
    }
}")

;;
;; bitfield manipulation optimizations
;;

(define_peephole	;; movstrqi+10
  [
   (set (reg:HI RAH) (zero_extend:HI (match_operand:QI 0 "general_operand" "gmf,c")))

   (set (reg:HI RAH)
        (lshiftrt:HI (reg:HI RAH)
                     (match_operand 1 "immediate_operand" "i,i")))

   (set (reg:QI RAH) (truncate:QI (reg:HI RAH)))

   (set (reg:QI RAH) (and:QI (reg:QI RAH)
                    (match_operand 2 "immediate_operand" "i,i")))
   (set (cc0)
	(reg:QI RAH))

   (set (pc)
        (if_then_else (eq (cc0) (const_int 0))
            (label_ref (match_operand 3 "" ""))
            (pc)))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE) && XAP_FLAG_ENABLED(BITFIELD_OPT) && (INTVAL(operands[2])==1) && dead_or_set_regno_p(PREV_INSN(insn), RAH)"
  "*
{
operands[4] = gen_rtx(REG, QImode, RAH);
operands[5] = gen_rtx(REG, QImode, RSCRATCH);
switch(which_alternative)
   {
   case 0: return \"; peephole12 here\;ld\\t%4,%f0\;asr\\t%1+17\;bcc\\t%l3\";
   case 1: 
      switch(REGNO(operands[0]))
         {
         case RAL: return \"; peephole12 here\;asr\\t%1+1\;bcc\\t%l3\";
         case RAH: return \"; peephole12 here\;asr\\t%1+17\;bcc\\t%l3\";
         case RXL: return \"; peephole12 here\;st\\t%0,%5\;ld\\t%4,%5\;asr\\t%1+17\;bcc\\t%l3\";
         default: abort();
         }
   }
}")

(define_peephole	;; movstrqi+11
  [
   (set (reg:HI RAH) (zero_extend:HI (match_operand:QI 0 "general_operand" "gmf,c")))

   (set (reg:HI RAH)
        (lshiftrt:HI (reg:HI RAH)
                     (match_operand 1 "immediate_operand" "i,i")))

   (set (reg:QI RAH) (truncate:QI (reg:HI RAH)))

   (set (reg:QI RAH) (and:QI (reg:QI RAH)
                    (match_operand 2 "immediate_operand" "i,i")))

   (set (cc0) 
	(reg:QI RAH))

   (set (pc)
        (if_then_else (ne (cc0) (const_int 0))
            (label_ref (match_operand 3 "" ""))
            (pc)))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE) && XAP_FLAG_ENABLED(BITFIELD_OPT) && (INTVAL(operands[2])==1) && dead_or_set_regno_p(PREV_INSN(insn), RAH)"
  "*
{
operands[4] = gen_rtx(REG, QImode, RAH);
operands[5] = gen_rtx(REG, QImode, RSCRATCH);
switch(which_alternative)
   {
   case 0: return \"; peephole12 here\;ld\\t%4,%f0\;asr\\t%1+17\;bcs\\t%l3\";
   case 1: 
      switch(REGNO(operands[0]))
         {
         case RAL: return \"; peephole12 here\;asr\\t%1+1\;bcs\\t%l3\";
         case RAH: return \"; peephole12 here\;asr\\t%1+17\;bcs\\t%l3\";
         case RXL: return \"; peephole12 here\;st\\t%0,%5\;ld\\t%4,%5\;asr\\t%1+17\;bcs\\t%l3\";
         default: abort();
         }
   }
}")

(define_peephole	;; movstrqi+12
  [
   (set (reg:HI RAH) (zero_extend:HI (match_operand:QI 0 "general_operand" "c,gmf")))

   (set (reg:HI RAH)
        (lshiftrt:HI (reg:HI RAH)
                     (match_operand 1 "immediate_operand" "i,i")))

   (set (reg:QI RAH) (truncate:QI (reg:HI RAH)))

   (set (cc0) 
	(reg:QI RAH))

   (set (pc)
        (if_then_else (eq (cc0) (const_int 0))
            (label_ref (match_operand 2 "" ""))
            (pc)))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE) && XAP_FLAG_ENABLED(BITFIELD_OPT) && dead_or_set_regno_p(PREV_INSN(insn), RAH)"
  "*
{
operands[4] = gen_rtx(REG, QImode, RAH);
operands[5] = gen_rtx(REG, QImode, RSCRATCH);
operands[6] = const0_rtx;
if (INTVAL(operands[1])==15) switch(which_alternative)
   {
   case 0:
      return \"; peephole22 here\;cmp\\t%0,%6\;bpl\\t%l2\";
   case 1: 
      return \"; peephole22 here\;tst\\t%0\;bpl\\t%l2\";
   }
else
   {
   operands[7] = GEN_INT((-1<<INTVAL(operands[1]))&0xFFFF);
   if (!which_alternative) switch(REGNO(operands[0]))
          {
          case RAH:
          case RAL: return \"; peephole21 here\;and\\t%0,%7\;beq\\t%l2\";
          case RXL: if (XAP_FLAG_ENABLED(ENHANCED))
                        return \"; peephole21 here\;ld\\t%4,%f0\;and\\t%4,%7\;beq\\t%l2\";
                    else
                        return \"; peephole21 here\;st\\t%0,%5\;ld\\t%4,%5\;and\\t%4,%7\;beq\\t%l2\";
          default: abort();
          }
   else
         {
         return \"; peephole21 here\;ld\\t%4,%0\;and\\t%4,%7\;beq\\t%l2\";
         }
   }
}")

(define_peephole	;; movstrqi+13
  [
   (set (reg:HI RAH) (zero_extend:HI (match_operand:QI 0 "general_operand" "c,gmf")))

   (set (reg:HI RAH)
        (lshiftrt:HI (reg:HI RAH)
                     (match_operand 1 "immediate_operand" "i,i")))

   (set (reg:QI RAH) (truncate:QI (reg:HI RAH)))

   (set (cc0) 
	(reg:QI RAH))

   (set (pc)
        (if_then_else (ne (cc0) (const_int 0))
            (label_ref (match_operand 2 "" ""))
            (pc)))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE) && XAP_FLAG_ENABLED(BITFIELD_OPT) && dead_or_set_regno_p(PREV_INSN(insn), RAH)"
  "*
{
operands[4] = gen_rtx(REG, QImode, RAH);
operands[5] = gen_rtx(REG, QImode, RSCRATCH);
operands[6] = const0_rtx;
if (INTVAL(operands[1])==15) switch(which_alternative)
   {
   case 0:
      return \"; peephole22 here\;cmp\\t%0,%6\;bmi\\t%l2\";
   case 1: 
      return \"; peephole22 here\;tst\\t%0\;bmi\\t%l2\";
   }
else
   {
   operands[7] = GEN_INT((-1<<INTVAL(operands[1]))&0xFFFF);
   if (!which_alternative) switch(REGNO(operands[0]))
          {
          case RAH:
          case RAL: return \"; peephole21 here\;and\\t%0,%7\;bne\\t%l2\";
          case RXL: if (XAP_FLAG_ENABLED(ENHANCED))
                        return \"; peephole21 here\;ld\\t%4,%f0\;and\\t%4,%7\;bne\\t%l2\";
                    else
                        return \"; peephole21 here\;st\\t%0,%5\;ld\\t%4,%5\;and\\t%4,%7\;bne\\t%l2\";
          default: abort();
          }
   else
         {
         return \"; peephole21 here\;ld\\t%4,%0\;and\\t%4,%7\;bne\\t%l2\";
         }
   }
}")

(define_peephole	;; movstrqi+14
  [
   (set (reg:HI RAH) (zero_extend:HI (match_operand:QI 0 "general_operand" "gmf,c")))

   (set (reg:HI RAH)
        (lshiftrt:HI (reg:HI RAH)
                     (match_operand 1 "immediate_operand" "i,i")))

   (set (reg:QI RAH) (truncate:QI (reg:HI RAH)))

   (set (reg:QI RAH) (and:QI (reg:QI RAH)
                    (match_operand 2 "immediate_operand" "i,i")))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE) && XAP_FLAG_ENABLED(BITFIELD_OPT) && (INTVAL(operands[2])<0x7fff)"
  "*
{
operands[3] = gen_rtx(REG, QImode, RAH);
operands[4] = gen_rtx(REG, QImode, RSCRATCH);
switch(which_alternative)
   {
   case 0: return \"; peephole14 here\;ld\\t%3,%f0\;asr\\t%1\;and\\t%3,%2\";
   case 1: 
      switch(REGNO(operands[0]))
         {
         case RAL: operands[5] = GEN_INT(16-INTVAL(operands[1]));
                   return \"; peephole14 here\;asl\\t%5\;and\\t%3,%2\";
         case RAH: return \"; peephole14 here\;asr\\t%1\;and\\t%3,%2\";
         case RXL: return \"; peephole14 here\;st\\t%0,%4\;ld\\t%3,%4\;asr\\t%1\;and\\t%3,%2\";
         default: abort();
         }
   }
}")

(define_peephole	;; movstrqi+15
  [
   (set (reg:HI RAH) (sign_extend:HI
	(sign_extract:QI (match_operand:QI 0 "general_operand" "gmif")
                         (match_operand 1 "immediate_operand" "")
                         (match_operand 2 "immediate_operand" ""))))
   (set (reg:QI RAH) (truncate:QI (reg:HI RAH)))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE) && XAP_FLAG_ENABLED(BITFIELD_OPT)"
  "*
{
operands[3] = GEN_INT(32-INTVAL(operands[1])-INTVAL(operands[2])); 
operands[4] = GEN_INT(16-INTVAL(operands[1]));
operands[5] = gen_rtx(REG, QImode, RAL);
return \"; peephole15 here\;; extvah\\t%0,%1,%2\;ld\\t%5,%f0\;asl\\t%3\;asr\\t%4\";
}")

(define_peephole	;; movstrqi+16
  [
   (set (reg:HI RAH) (zero_extend:HI
	(zero_extract:QI (match_operand:QI 0 "general_operand" "gmif")
                         (match_operand 1 "immediate_operand" "")
                         (match_operand 2 "immediate_operand" ""))))
   (set (reg:QI RAH) (truncate:QI (reg:HI RAH)))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE) && XAP_FLAG_ENABLED(BITFIELD_OPT)"
  "*
{
operands[3] = GEN_INT(33-INTVAL(operands[1])-INTVAL(operands[2])); 
operands[4] = GEN_INT(17-INTVAL(operands[1]));
operands[5] = gen_rtx(REG, HImode, RAL);
return \"; peephole16 here\;; extzvah\\t%0,%1,%2\;ld\\t%5,%f0\;asl\\t%3\;ror\\t%4\";
}")

(define_peephole	;; movstrqi+18
  [
    (set (reg:HI RAH) (zero_extend:HI (reg:QI RAH)))
    (set (reg:HI RAH)
         (lshiftrt:HI (reg:HI RAH)
                      (match_operand 0 "immediate_operand" "i")))
    (set (reg:QI RAH) (truncate:QI (reg:HI RAH)))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE) && XAP_FLAG_ENABLED(BITFIELD_OPT)"
  "lsr\\t%f0")

(define_peephole	;; movstrqi+19
  [
    (set (reg:HI RAH) (zero_extend:HI (reg:QI RAH)))
    (set (reg:HI RAH)
         (lshiftrt:HI (reg:HI RAH)
                      (match_operand 0 "immediate_operand" "i")))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE) && XAP_FLAG_ENABLED(BITFIELD_OPT)"
  "; peephole18 here\;lsr\\t%0+16")

(define_peephole	;; movstrqi+20
  [
    (set (reg:HI RAH) (zero_extend:HI (match_operand:QI 0 "memory_operand" "gm")))
    (set (reg:HI RAH)
         (lshiftrt:HI (reg:HI RAH)
                      (match_operand 1 "immediate_operand" "i")))
    (set (reg:QI RAH) (truncate:QI (reg:HI RAH)))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE) && XAP_FLAG_ENABLED(BITFIELD_OPT)"
  "*
{
operands[2] = gen_rtx(REG, QImode, RAH);
return \"; peephole19 here\;ld\\t%2,%0\;lsr\\t%f1\";
}")

(define_peephole	;; movstrqi+21
  [
    (set (reg:QI RAH) (and:QI (reg:QI RAH) (match_operand 0 "immediate_operand" "i")))
    (set (reg:HI RAH)
         (zero_extend:HI (reg:QI RAH)))
  ]
  "XAP_FLAG_ENABLED(PEEPHOLE) && (INTVAL(operands[0])<0x7fff)"
  "*
{
operands[2] = gen_rtx(REG, QImode, RAH);
operands[3] = GEN_INT(16);
return \"; peephole20 here\;and\\t%2,%0\;asr\\t%3\";
}")

(define_peephole2
  [
    (set (reg:QI RXL) (reg:QI RAL))
    (match_scratch:QI 0 "d")
    (set (reg:QI RAL) (match_operand:QI 1 "general_operand" ""))
    (set (match_operand:QI 2 "memory_operand" "") (reg:QI RAL))
    (match_dup 0)
    (set (reg:QI RAL) (reg:QI RXL))
  ]
  ""
  [
    (set (reg:QI RXL) (reg:QI RAL))
    (set (match_dup 0) (match_operand:QI 1 "general_operand" ""))
    (set (match_operand:QI 2 "memory_operand" "") (match_dup 0))
  ]
  "")

(define_peephole2
  [
    (set (reg:QI RXL) (reg:QI RAL))
    (match_scratch:QI 0 "d")
    (set (reg:QI RAL) (match_operand:QI 1 "general_operand" ""))
    (set (match_operand:QI 2 "memory_operand" "") (reg:QI RAL))
    (set (reg:QI RAL) (match_operand:QI 3 "general_operand" ""))
    (set (match_operand:QI 4 "memory_operand" "") (reg:QI RAL))
    (match_dup 0)
    (set (reg:QI RAL) (reg:QI RXL))
  ]
  ""
  [
    (set (reg:QI RXL) (reg:QI RAL))
    (set (match_dup 0) (match_operand:QI 1 "general_operand" ""))
    (set (match_operand:QI 2 "memory_operand" "") (match_dup 0))
    (set (match_dup 0) (match_operand:QI 3 "general_operand" ""))
    (set (match_operand:QI 4 "memory_operand" "") (match_dup 0))
  ]
  "")

(define_split
  [
    (set (match_operand:HI 0 "register_operand" "")
         (minus:HI (match_dup 0)
                   (match_dup 0)))
  ]
  ""
  [
    (set (match_dup 0) (const_int 0))
  ]
  ""
)

; vim: set ft=lisp :


