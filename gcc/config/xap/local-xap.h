/*
 * Definitions of target machine for GNU compiler, for the Xap JBS 24 Oct 2000,
 * based on JRRK's XAP description and the PDP11 configuration
 */

#ifndef __xap2_plus_H
#define __xap2_plus_H

extern int target_flags;
extern int xap_target_default;

#define OPTIMIZATION_OPTIONS(optimize, optimize_size) \
{ \
target_flags = xap_target_default; \
if (!optimize) \
    { \
      optimize = 1; \
      flag_merge_constants = 1; \
      flag_defer_pop = 1; \
      flag_thread_jumps = 1; \
      flag_guess_branch_prob = 1; \
      flag_cprop_registers = 1; \
      flag_loop_optimize = 1; \
      flag_crossjumping = 1; \
      flag_if_conversion = 1; \
      flag_if_conversion2 = 1; \
    } \
} \

/* Macro to define tables used to set the flags.
   This is a list in braces of triplets in braces,
   each triplet being { "NAME", VALUE, DOC }
   where VALUE is the bits to set or minus the bits to clear and DOC
   is the documentation for --help (NULL if intentionally undocumented).
   An empty string NAME is used to identify the default VALUE.  */

#define XAP_USE_NO(macro,a,b,c) macro(a,b, "Use " c, "No " c)

#define XAP_FLAG_TABLE(macro) \
    macro(SMALL, "small-class", "Minimise register lifetimes",     \
                                "Allow longer register lifetimes") \
    macro(UNDERSCORE, "underscore", "Prepend underscore",     \
                                    "No prepend underscore") \
    XAP_USE_NO(macro,PEEPHOLE, "peephole", "peephole mode") \
    XAP_USE_NO(macro, PEEPHOLE2, "peephole2", "peephole2 mode") \
    XAP_USE_NO(macro, XAP2, "xap2-mode", "xap2 mode") \
    XAP_USE_NO(macro, LARGE, "large-mode", "large mode") \
    XAP_USE_NO(macro, FLOAT, "float-mode", "float mode") \
    XAP_USE_NO(macro, DOUBLE, "double-mode", "double mode") \
    XAP_USE_NO(macro, QUIET, "quiet-mode", "double mode") \
    XAP_USE_NO(macro, BLOCK_COPY, "inline-block-copy-mode", "block copy mode") \
    XAP_USE_NO(macro, ENHANCED, "enhanced-mode", "enhanced mode") \
    XAP_USE_NO(macro, FRAME_OPTIM, "frame-optim-mode", "frame optimization mode") \
    XAP_USE_NO(macro, TEST_OPTIM, "test-optim-mode", "test optimization mode") \
    XAP_USE_NO(macro, LOAD_CACHE, "load-cache-mode", "load cache mode") \
    XAP_USE_NO(macro, BITFIELD_OPT, "bitfield-mode", "bitfield opt mode") \
    macro(PUSH_REVERSED, "push-reversed-mode", "Push function arguments in reverse order",  \
                                               "Push function arguments in normal order" ) \
    XAP_USE_NO(macro, FUNC_PTR_OPT, "func-ptr-opt-mode", "function pointer optimisation") \
    macro(FUNC_PTR_INIT, "func-ptr-init-mode", "Place function pointers in initialised data segment", \
                                               "Place function pointers in constant segment")        \
    macro(SWITCH_TABLE, "switch-table-code-mode", "Place switch table in code segment",       \
                                                  "Place switch tables in constant segment") \
    XAP_USE_NO(macro, BIG_FUNCTION_PTR, "big-function-pointers", "big function pointers") \
    macro(TRULY_NOP_TRUNC, "truly-nop-truncation", "Truly nop truncation mode", \
                                                   "No Truly nop truncation mode") \
    macro(LEAF_OPTIM, "leaf-optim", "Optimize leaf functions", \
                                    "No extra leaf function optimization") \
    XAP_USE_NO(macro, TEST3, "test3", "Test mode 3") \
    XAP_USE_NO(macro, TEST4, "test4", "Test mode 4") \
    XAP_USE_NO(macro, MANGLED_MODULE_NAMES, "mangled-module-names", "mangled module names") \
    macro(LONG_ARGS_STRADDLE, "args-span-regs-and-mem", "32 bit arguments can be split between register and stack", \
                                                        "32 bit arguments either go in registers or on the stack") \
    macro(STABS, "stabs", "Don't disable stabs", "Disable stabs")

#define XAP_FLAG_POSN_ENUM_X(a,b,c,d) XAP_FLAG_ ## a ## _POSN ,
enum xap_flag_enum
{
    XAP_FLAG_TABLE(XAP_FLAG_POSN_ENUM_X)
    XAP_TARGET_SWITCH_POSN_ENUM_NUM
};

#define XAP_FLAG_MASK(x) (1 << XAP_FLAG_ ## x ## _POSN)
#define TARGET_SWITCHES_X(a,b,c,d) \
    { b, XAP_FLAG_MASK(a), c }, \
    { "no-" b, -XAP_FLAG_MASK(a), d },

#define TARGET_SWITCHES \
{ \
    XAP_FLAG_TABLE(TARGET_SWITCHES_X) \
    { "", 0, NULL} \
}

#define XAP_FLAG_ENABLED(x) (XAP_FLAG_MASK(x)&target_flags)


/* target options structure as needed by gcc.c

static const struct {
  const char *const option_found;
  const char *const replacements;
} target_option_translations[] =
{
  TARGET_OPTION_TRANSLATE_TABLE,
  { 0, 0}
};

*/

#define TARGET_OPTION_TRANSLATE_TABLE   { "-help", "-dumpmachine" }, \
					{ "-xap2", "-mxap2-mode" }, \
					{ "-xap2+", "-mxap2-mode -menhanced-mode" }, \
					{ "-quiet", "-mquiet-mode" }, \
					{ "-float", "-mfloat-mode" }, \
					{ "-double","-mdouble-mode" }, \
					{ "-large", "-mlarge-mode" }

/* Type sizes */

#define CHAR_TYPE_SIZE		16
#define SHORT_TYPE_SIZE		16
#define INT_TYPE_SIZE		16
#define LONG_TYPE_SIZE		32
#define LONG_LONG_TYPE_SIZE	64
#define MAX_FIXED_MODE_SIZE     64

#define FLOAT_TYPE_SIZE		32
#define DOUBLE_TYPE_SIZE	64
#define LONG_DOUBLE_TYPE_SIZE   64

/* #define REAL_ARITHMETIC */

#define TARGET_FLOAT_FORMAT     IEEE_FLOAT_FORMAT
#define WIDEST_HARDWARE_FP_SIZE 64

/* machine types from ansi */

#define SIZE_TYPE               "unsigned int"
#define WCHAR_TYPE              "int"
#define WCHAR_TYPE_SIZE         16
#define PTRDIFF_TYPE            "int"

/* target machine storage layout */

extern void *expr;

#define BITS_BIG_ENDIAN         0
#define BYTES_BIG_ENDIAN        1
#define WORDS_BIG_ENDIAN        1
#define LIBGCC2_WORDS_BIG_ENDIAN 1
#define FLOAT_WORDS_BIG_ENDIAN  1
#define BITS_PER_WORD           16
#define UNITS_PER_WORD          1
#define POINTER_BOUNDARY        16
#define PARM_BOUNDARY           16
#define FUNCTION_BOUNDARY       16
#define EMPTY_FIELD_BOUNDARY    16
#define STACK_BOUNDARY		16
#define BIGGEST_ALIGNMENT       16
#define STRICT_ALIGNMENT        0

#define BITFIELD_NBYTES_LIMITED 1

/* Standard register usage.  */

/* Number of actual hardware registers.
   The hardware registers are assigned numbers for the compiler
   from 0 to just below FIRST_PSEUDO_REGISTER.
   All registers that the compiler knows about must be given numbers,
   even those that are not normally considered general registers.

   All used by call

           AH AL XH XL AP SP FP FL scratch
   Number   0  1  2  3  4  5  6  7 8..14 15
   Fixed             +  +  +  +  +       +
*/

#include "insn-codes.h"

/* overridden in INIT_ONCE_REG_SET() */

char fixed_regs[FIRST_PSEUDO_REGISTER];
char call_used_regs[FIRST_PSEUDO_REGISTER];

#define FIXED_REGISTERS {0}
#define CALL_USED_REGISTERS {0}

#define REG_ALLOC_ORDER {};

/* Return number of consecutive hard regs needed starting at reg REGNO
   to hold something of mode MODE.
*/

int hard_regno_nregs(int, int);

#define HARD_REGNO_NREGS(REGNO, MODE) hard_regno_nregs(REGNO, MODE)

/* Value is 1 if hard register REGNO can hold a value of machine-mode MODE. */

int hard_regno_mode_ok(int regno, int mode);

#define HARD_REGNO_MODE_OK(REGNO, MODE) hard_regno_mode_ok(REGNO, MODE)

/* Value is 1 if it is a good idea to tie two pseudo registers
   when one has mode MODE1 and one has mode MODE2.
   If HARD_REGNO_MODE_OK could produce different values for MODE1 and MODE2,
   for any hard reg, then this must be 0 for correct output.  */

int modes_tieable(int mode1, int mode2);

#define MODES_TIEABLE_P(MODE1, MODE2) modes_tieable(MODE1, MODE2)

/* Specify the registers used for certain standard purposes.
   The values of these macros are register numbers.  */

#define PC_REGNUM               7
#define FRAME_POINTER_REGNUM    6
#define STACK_POINTER_REGNUM    5
#define ARG_POINTER_REGNUM      4

#define FRAME_POINTER_REQUIRED  1

#define STRUCT_VALUE_REGNUM     8
#define STATIC_CHAIN_REGNUM     9

/* How to eliminate the fake AP and SP in terms of FP */

#define ELIMINABLE_REGS \
    { { RAP, RFP}, {RSP, RFP} }

int can_eliminate(int from, int to);

#define CAN_ELIMINATE(FROM, TO) can_eliminate(FROM, TO)

int initial_elimination_offset(int from, int to);

#define INITIAL_ELIMINATION_OFFSET(FROM,TO,OFFSET) \
    ((OFFSET) = initial_elimination_offset(FROM, TO))

/* Define the classes of registers for register constraints in the
   machine description.  Also define ranges of constants.

   One of the classes must always be named ALL_REGS and include all hard regs.
   If there is more than one class, another class must be named NO_REGS
   and contain no registers.

   The name GENERAL_REGS must be the name of a class (or an alias for
   another name such as ALL_REGS).  This is the class of registers
   that is allowed by "g" or "r" in a register constraint.
   Also, registers outside this class are allocated only when
   instructions express preferences for them.

   The classes must be numbered in nondecreasing order; that is,
   a larger-numbered class must never be contained completely
   in a smaller-numbered class.

   For any two classes, it is very desirable that there be another
   class that represents their union.  */
   
enum reg_class { NO_REGS, ADDR_REGS, FRAME_REGS, DATA_REGS, CHIP_REGS, FAKE_REGS, ALL_REGS, LIM_REG_CLASSES };

#define N_REG_CLASSES          ((int) LIM_REG_CLASSES)

#define SMALL_REGISTER_CLASSES XAP_FLAG_ENABLED(SMALL)
#define GENERAL_REGS           NO_REGS

/* Give names of register classes as strings for dump file.   */

#define REG_CLASS_NAMES {"NO_REGS", "ADDR_REGS", "FRAME_REGS", "DATA_REGS", "CHIP_REGS", "FAKE_REGS", "ALL_REGS" }

/* Define which registers fit in which classes.
   This is an initializer for a vector of HARD_REG_SET
   of length N_REG_CLASSES.  */

#define REG_CLASS_CONTENTS {{}}

/* The same information, inverted:
   Return the class number of the smallest class containing
   reg number REGNO.  This could be a conditional expression
   or could index an array.  */

extern int regno_reg_class(int);

#define REGNO_REG_CLASS(REGNO) regno_reg_class(REGNO)

/* The class value for index registers, and the one for base regs.  */
#define INDEX_REG_CLASS NO_REGS
#define BASE_REG_CLASS  ADDR_REGS

/* Get reg_class from a letter such as appears in the machine description.  */

extern int reg_class_from_letter(int c);

#define REG_CLASS_FROM_LETTER(C) \
  reg_class_from_letter(C)

/* The letters I and J in a register constraint string
   can be used to stand for particular ranges of immediate operands.
   This macro defines what the ranges are.
   C is the letter, and VALUE is a constant value.
   Return 1 if VALUE is in the range specified by C.

   I		-128 to 127
   J		-32768 to 32767
*/

int xap_const_ok_for_letter_p(int value, int letter);

#define CONST_OK_FOR_LETTER_P(VALUE, C)  xap_const_ok_for_letter_p(VALUE, C)

#define CONST_DOUBLE_OK_FOR_LETTER_P(VALUE, C)  \
    0

/* Given an rtx X being reloaded into a reg required to be
   in class CLASS, return the class of reg to actually use.
   In general this is just CLASS; but on some machines
   in some cases it is preferable to use a more restrictive class.  
*/

#define PREFERRED_RELOAD_CLASS(X,CLASS) preferred_reload_class(X,CLASS)
#define SECONDARY_INPUT_RELOAD_CLASS(CLASS, MODE, IN) xap_secondary_input_reload_class(CLASS, MODE, IN)
#define SECONDARY_OUTPUT_RELOAD_CLASS(CLASS, MODE, IN) xap_secondary_output_reload_class(CLASS, MODE, IN)
#define SECONDARY_MEMORY_NEEDED(CLASS1, CLASS2, MODE) xap_secondary_memory_needed(CLASS1, CLASS2, MODE)
#define SECONDARY_MEMORY_NEEDED_RTX(MODE) xap_secondary_memory_needed_rtx(MODE)

#define CLASS_LIKELY_SPILLED_P(CLASS) class_likely_spilled(CLASS)

/* Return the maximum number of consecutive registers
   needed to represent mode MODE in a register of class CLASS.  */

#define CLASS_MAX_NREGS(CLASS, MODE) \
    ((GET_MODE_SIZE (MODE) + UNITS_PER_WORD - 1) / UNITS_PER_WORD)


/* Stack layout; function entry, exit and calling.  */

#define STACK_GROWS_DOWNWARD
/*	#define FRAME_GROWS_DOWNWARD	*/
#define ACCUMULATE_OUTGOING_ARGS               1

#define STARTING_FRAME_OFFSET                  current_function_outgoing_args_size
#define FIRST_PARM_OFFSET(DECL)                0
#define RETURN_POPS_ARGS(FUNDECL,FUNTYPE,SIZE) 0

void *libcall_value(int m);

#define LIBCALL_VALUE(MODE) libcall_value(MODE)

#define FUNCTION_VALUE(VALTYPE, FUNC)  \
    LIBCALL_VALUE(TYPE_MODE(VALTYPE))

#define FUNCTION_OUTGOING_VALUE(VALTYPE, FUNC)  \
    LIBCALL_VALUE(TYPE_MODE(VALTYPE))

#define FUNCTION_VALUE_REGNO_P(N) ((N)==RAL || (N)==RAH)

#define RETURN_IN_MEMORY(TYPE) xap2plus_return_in_memory(TYPE)

#define FUNCTION_ARG_REGNO_P(N) \
    ((N)==RAL || (N)==RAH) /*FIX: static chain or structure value address? */


/* Define a data type for recording info about an argument list
   during the scan of that argument list.  This data type should
   hold all necessary information about the function itself
   and about the args processed so far, enough to enable macros
   such as FUNCTION_ARG to determine where the next arg should go.
*/

#define CUMULATIVE_ARGS int

/* Update the data in CUM to advance over an argument
   of mode MODE and data type TYPE.
   (TYPE is null for libcalls where that information may not be available.)  
*/

#define ARGUMENT_SIZE(MODE, TYPE) \
  ((MODE) != BLKmode ? GET_MODE_SIZE(MODE) : int_size_in_bytes(TYPE))

#define FUNCTION_ARG_ADVANCE(CUM, MODE, TYPE, NAMED) \
    ((CUM) += ARGUMENT_SIZE(MODE,TYPE))

/* Determine where to put an argument to a function.
   Value is zero to push the argument on the stack,
   or a hard register in which to store the argument.

   MODE is the argument's machine mode.
   TYPE is the data type of the argument (as a tree).
    This is null for libcalls where that information may
    not be available.
   CUM is a variable of type CUMULATIVE_ARGS which gives info about
    the preceding args and about the function being called.
   NAMED is nonzero if this argument is a named parameter
    (otherwise it is an extra parameter matching an ellipsis).  */

#define FUNCTION_ARG(CUM, MODE, TYPE, NAMED) function_arg(CUM,MODE,TYPE,NAMED)
#define FUNCTION_ARG_PARTIAL_NREGS(CUM, MODE, TYPE, NAMED) \
    xap_function_arg_partial_nregs(CUM,MODE,TYPE,NAMED)

#define EXIT_IGNORE_STACK \
    0

#define INITIAL_FRAME_POINTER_OFFSET(DEPTH) \
    ((DEPTH) = 0)
    
#define FUNCTION_PROFILER(FILE, LABELNO)

#define TARGET_MEM_FUNCTIONS 1


/* Addressing modes, and classification of registers for them.  */

#define HAVE_POST_INCREMENT 0
#define HAVE_PRE_DECREMENT  0

/* Macros to check register numbers against specific register classes.  */

/* These assume that REGNO is a hard or pseudo reg number.
   They give nonzero only if REGNO is a hard reg of the suitable class
   or a pseudo reg currently allocated to a suitable hard reg.
   Since they use reg_renumber, they are safe only once reg_renumber
   has been allocated, which happens in local-alloc.c.  */

#define REGNO_OK_FOR_INDEX_P(REGNO) \
    0

#define HARD_REGNO_OK_FOR_BASE_P(REGNO) \
    ((REGNO)==RXL || (REGNO)==RSP || (REGNO)==RAP || (REGNO)==RFP)

#define REGNO_OK_FOR_BASE_P(REGNO) \
    (HARD_REGNO_OK_FOR_BASE_P(REGNO) || (reload_completed && HARD_REGNO_OK_FOR_BASE_P(reg_renumber[REGNO])))

/* Maximum number of registers that can appear in a valid memory address.  */

#define MAX_REGS_PER_ADDRESS 1

/* Recognize any constant value that is a valid address.  */

#define CONSTANT_ADDRESS_P(X) \
    CONSTANT_P(X)

/* Nonzero if the constant value X is a legitimate general operand.
   It is given that X satisfies CONSTANT_P or is a CONST_DOUBLE.  */

#define LEGITIMATE_CONSTANT_P(X) \
    (GET_CODE(X) != CONST_DOUBLE)

/* The macros REG_OK_FOR..._P assume that the arg is a REG rtx
   and check its validity for a certain class.
   We have two alternate definitions for each of them.
   The usual definition accepts all pseudo regs; the other rejects
   them unless they have been allocated suitable hard regs.
   The symbol REG_OK_STRICT causes the latter definition to be used.

   Most source files want to accept pseudo regs in the hope that
   they will get allocated to the class that the insn wants them to be in.
   Source files for reload pass need to be strict.
   After reload, it makes no difference, since pseudo regs have
   been eliminated by then.  */

#ifndef REG_OK_STRICT
#define REG_OK_FOR_INDEX_P(X) reg_ok_for_base_p(X, FALSE)
#define REG_OK_FOR_BASE_P(X) reg_ok_for_base_p(X, FALSE)
#define GO_IF_LEGITIMATE_ADDRESS(mode, X, ADDR) \
           if (legitimate_address(mode, X, FALSE)) goto ADDR
#else
#define REG_OK_FOR_INDEX_P(X) reg_ok_for_base_p(X, TRUE)
#define REG_OK_FOR_BASE_P(X) reg_ok_for_base_p(X, TRUE)
#define GO_IF_LEGITIMATE_ADDRESS(mode, X, ADDR) \
           if (legitimate_address(mode, X, TRUE)) goto ADDR
#endif

#define LEGITIMIZE_ADDRESS(X,OLDX,MODE,WIN) \
if (legitimize_address(X,OLDX,MODE)) goto WIN

/* Go to LABEL if ADDR (a legitimate address expression)
   has an effect that depends on the machine mode it is used for.
   On the pdp this is for predec/postinc */

extern int xap_mode_dependent_address(void *);

#define GO_IF_MODE_DEPENDENT_ADDRESS(ADDR,LABEL) if (xap_mode_dependent_address(ADDR)) goto LABEL


/* Specify the machine mode that this machine uses
   for the index in the tablejump instruction.  */
#define CASE_VECTOR_MODE QImode

/* Define as C expression which evaluates to nonzero if the tablejump
   instruction expects the table to contain offsets from the address of the
   table.
   Do not define this if the table should contain absolute addresses. */
#define CASE_VECTOR_PC_RELATIVE 1

/* Define this as 1 if `char' should by default be signed; else as 0.  */
#define DEFAULT_SIGNED_CHAR 0

/* Max number of bytes we can move from memory to memory
   in one reasonably fast instruction.  */

#define MOVE_MAX 2

#define CLEAR_RATIO 3

/* Zero extension is faster if the target is known to be zero */
/* #define SLOW_ZERO_EXTEND */

/* Nonzero if access to memory by byte is slow and undesirable. -
*/
#define SLOW_BYTE_ACCESS 0

/* Do not break .stabs pseudos into continuations.  */
#define DBX_CONTIN_LENGTH 80

/* Value is 1 if truncating an integer of INPREC bits to OUTPREC bits
   is done just by pretending it is already truncated.  */

int truly_noop_truncation(int outprec, int inprec);

#define TRULY_NOOP_TRUNCATION(outprec, inprec) truly_noop_truncation(outprec, inprec)

/* We assume that the store-condition-codes instructions store 0 for false
   and some other value for true.  This is the value stored for true.  */

#define STORE_FLAG_VALUE -1
/* #define POINTERS_EXTEND_UNSIGNED */
#undef POINTERS_EXTEND_UNSIGNED

#define POINTER_SIZE_(x) xap_pointer_size(NULL,x,__FILE__,__LINE__)
#define Pmode_(x) xap_pmode(NULL,x,__FILE__,__LINE__)

#define POINTER_SIZE xap_pointer_size(decl,type,__FILE__,__LINE__)
#ifdef REG_OK_STRICT
#define Pmode xap_pmode(decl,NULL,__FILE__,__LINE__)
#else
#define Pmode xap_pmode(decl,type,__FILE__,__LINE__)
#endif
#define FUNCTION_MODE (HImode)

#define NO_FUNCTION_CSE 1

/* cost of moving one register class to another */

#define CC_NO_OVER_OR_CARRY 01000
#define NOTICE_UPDATE_CC(EXP,INSN) notice_update_cc()


/* Control the assembler format that we output.  */

/* Define results of standard character escape sequences.  */
#define TARGET_BELL    007
#define TARGET_BS      010
#define TARGET_TAB     011
#define TARGET_NEWLINE 012
#define TARGET_VT      013
#define TARGET_FF      014
#define TARGET_CR      015
#define TARGET_ESC     033

/* Output to assembler file text saying following lines
   may contain character constants, extra white space, comments, etc.  */

extern const char *asm_app_on(void);
extern const char *asm_app_off(void);
extern const char *text_section_asm_op(void);
extern const char *data_section_asm_op(void);
extern const char *bss_section_asm_op(void);
extern const char *asm_comment_start(void);
extern const char *const_section_asm_op(void);
extern const char *initc_section_asm_op(void);
extern const char *xconst_section_asm_op(void);

#define ASM_APP_ON              asm_app_on()

/* Output to assembler file text saying following lines
   no longer contain unusual constructs.  */

#define ASM_APP_OFF             asm_app_off()

/* Output before read-only data.  */

#define TEXT_SECTION_ASM_OP     text_section_asm_op()

/* Output before writable initialised data  */

#define DATA_SECTION_ASM_OP     data_section_asm_op()

/* Ouput before writable uninitialised data */

#define BSS_SECTION_ASM_OP      bss_section_asm_op()

#define INITC_SECTION_ASM_OP    

#define EXTRA_SECTIONS         in_const,in_initc,in_xconst

#define READONLY_DATA_SECTION  const_section

#define ASM_COMMENT_START      asm_comment_start()

#define ASM_OUTPUT_SPECIAL_POOL_ENTRY(file, x, mode, align, labelno, done) \
    if (asm_output_special_pool_entry(file, x, mode, align, labelno)) goto done

extern void const_section(void);
extern void initc_section(void);
extern void xconst_section(void);
extern int push_section(void);
extern void pop_section(int);
int in_bss_section(void);
extern void xap2_plus_first_function(void);

#define EXTRA_SECTION_FUNCTIONS	\
  void const_section() \
  { \
    if (in_section != in_const) \
    { \
      xap2_plus_first_function(); \
      fprintf (asm_out_file, "%s\n", const_section_asm_op()); \
      in_section = in_const; \
    } \
  } \
  void initc_section() \
  { \
    if (in_section != in_initc) \
      { \
        xap2_plus_first_function(); \
        fprintf (asm_out_file, "%s\n", initc_section_asm_op()); \
        in_section = in_initc; \
      } \
  } \
  void xconst_section() \
  { \
    if (in_section != in_xconst) \
    { \
      xap2_plus_first_function(); \
      fprintf (asm_out_file, "%s\n", xconst_section_asm_op()); \
      in_section = in_xconst; \
    } \
  } \
  int push_section() \
  { \
    return (in_section); \
  } \
  void pop_section(int section) \
  { \
    if (in_section != section) switch (section) \
    { \
      case in_text:   text_section(); break; \
      case in_data:   data_section(); break; \
      case in_bss:    bss_section(); break; \
      case in_const:  const_section(); break; \
      case in_initc:  initc_section(); break; \
      case in_xconst: xconst_section(); break; \
      case no_section: break; \
    default: abort(); \
    } \
  } \
  int in_bss_section(void) \
  {\
      return in_section == in_bss; \
  }

/* How to refer to registers in assembler output.
   This sequence is indexed by compiler's hard-register-number (see above).  */

#define REGISTER_NAMES \
{ "AH", "AL", "XH", "X", "AP", "Y", "Y", "RSPECIAL", \
  "@H'fff8", "@H'fff9", "@H'fffa", "@H'fffb", \
  "@H'fffc", "@H'fffd", "@H'fffe", "@H'ffff", \
  "@(-1,Y)", "@(-2,Y)", "@(-3,Y)", "@(-4,Y)", \
  "@(-5,Y)", "@(-6,Y)", "@(-7,Y)", "@(-8,Y)"  \
}

/* This is how to store into the string LABEL
   the symbol_ref name of an internal numbered label where
   PREFIX is the class of label and NUM is the number within the class.
   This is suitable for output with `assemble_name'.  */

extern void asm_generate_internal_label(char *lab, const char *prefix, int num);

#define ASM_GENERATE_INTERNAL_LABEL(LABEL,PREFIX,NUM)	\
    asm_generate_internal_label(LABEL, PREFIX, NUM)

void output_addr_const_xap(void *f, void *value);

#define ASM_OUTPUT_CHAR(FILE,VALUE) \
    xap_assemble_integer(VALUE, sizeof(char), 0)

#define ASM_OUTPUT_BYTE(FILE,VALUE)  \
    fprintf(FILE, "\tdc\tH'%.4X\n", (VALUE))

#define ASM_OUTPUT_ASCII(FILE, P, SIZE)  \
    output_ascii(FILE, P, SIZE)

/* This is how to output an element of a case-vector that is absolute.  */

#define ASM_OUTPUT_ADDR_VEC_ELT(FILE, VALUE) output_addr_vec_elt(FILE, VALUE)

#define ASM_OUTPUT_ADDR_DIFF_ELT(FILE, BODY, VALUE1, VALUE2) \
   output_addr_diff_elt(FILE, BODY, VALUE1, VALUE2)

#define ASM_OUTPUT_CASE_LABEL(FILE, L, NUMBER, INSN) \
   output_case_label(FILE, L, NUMBER, INSN)

/* Store in OUTPUT a string (made with alloca) containing
   an assembler-name for a local static variable named NAME.
   LABELNO is an integer which is different for each call.  */

extern char *asm_format_private_name(void *, const char * const, int);

#define ASM_FORMAT_PRIVATE_NAME(OUTPUT, NAME, LABELNO) \
    OUTPUT = asm_format_private_name(alloca(strlen(NAME)+10), NAME, LABELNO)

#define PRINT_OPERAND(FILE, X, CODE)  \
    print_operand(FILE,X,CODE)

/* Print a memory address as an operand to reference that memory location.  */

#define PRINT_OPERAND_ADDRESS(FILE, ADDR)  \
    print_operand_address(FILE, ADDR)

#define ASM_OUTPUT_REG_PUSH(FILE,REGNO) \
    fprintf(FILE, "\tST\t%s,@(-1,Y)\n\tSUB\tY,#1\n", reg_names[REGNO])

#define ASM_OUTPUT_REG_POP(FILE,REGNO) \
    fprintf(FILE, "\tLD\t%s,@(0,Y)\n\tADD\tY,#1\n", reg_names[REGNO])


/*
  Trampolines are only used for nested functions.
  Don't care about supporting funny GNU extensions.
*/

#define TRAMPOLINE_TEMPLATE(FILE) \
    abort()

#define TRAMPOLINE_SIZE \
    0

#define INITIALIZE_TRAMPOLINE(TRAMP,FNADDR,CXT)	\
    abort()

/* Provide the costs of a rtl expression.  This is in the body of a
   switch on CODE. 
*/

#define BRANCH_COST        0

#define REGISTER_MOVE_COST(MODE, CLASS, DIRECTION) register_move_cost(MODE, CLASS, DIRECTION)

#define MEMORY_MOVE_COST(MODE,CLASS,IN) memory_move_cost(MODE, CLASS, IN)

/* Gross! We steal a pointer to a static funciton in toplev.c as well as doing
 * the sort of things that are normal for OVERRIDE_OPTIONS. We use the pointer
 * for dumping the command line flags active to the assembler file. */
#define OVERRIDE_OPTIONS override_options(print_switch_values)

#define INIT_SECTION_ASM_OP "brk"

/* now read the defaults */

#define DBX_REGISTER_NUMBER(regno) \
    (regno)

const char *xap_asm_stabs_op(void);
const char *xap_asm_stabd_op(void);
const char *xap_asm_stabn_op(void);

#define ASM_STABS_OP \
    xap_asm_stabs_op()

#define ASM_STABD_OP \
    xap_asm_stabd_op()

#define ASM_STABN_OP \
    xap_asm_stabn_op()

#define DBX_DEBUGGING_INFO
#define SDB_DEBUGGING_INFO

#include<stdio.h>
void xap2plus_dbx_function_end(FILE *f, tree func);
#define DBX_OUTPUT_FUNCTION_END(stream,function) xap2plus_dbx_function_end(stream,function)

extern int preferred_debugging_type;

#define PREFERRED_DEBUGGING_TYPE preferred_debugging_type

/* Debugging support */

extern char xap_cwd[2048];
extern char xap_file[2048];

void xap_set_cwd(const char *cwd);
void xap_set_file(const char *file);
void xap2_plus_output_source_line(unsigned int lineno, const char *xap_file);

#define DBX_OUTPUT_MAIN_SOURCE_DIRECTORY(asmfile, cwd) \
    xap_set_cwd(cwd)

#define DBX_OUTPUT_MAIN_SOURCE_FILENAME(asmfile, input_file_name) \
    xap_set_file(input_file_name)

/* Switch from crt0.o to gcrt0.o when debugging */

#define STARTFILE_SPEC \
    "crt0%O%s"

#define LIB_SPEC \
    "-lc"

#define LIBGCC_SPEC \
    ""

extern void xap2_plus_target_assert(int flags);
extern int dead_reg(void *insn);
extern int dead_regno(void *insn, int regno);
extern const char *xap2_plus_qi3 (const char *name,
				  void *operands,
				  void *insn,
				  int flags);
extern void *xap_gen_compare_reg(int relational, void *label);
extern void *xap_compare_op0, *xap_compare_op1, *saved_y_rtx;
extern void *xap2_plus_reg_rtx(int regno);
extern void machine_dep_reorg(void *insns);
extern void *xap2_plus_hard_split_reg;
extern int hard_split_reg(void *x);
extern int xap2_plus_dead;
extern void xap_test_optim(rtx *operands);

#define CONSTANT_POOL_BEFORE_FUNCTION 0

/* Handle XAP compiler's pragmas.  */

#define REGISTER_TARGET_PRAGMAS register_target_pragmas

void cc_status_mdep_init(void);

#define CC_STATUS_MDEP cc_status_mdep
#define CC_STATUS_MDEP_INIT cc_status_mdep_init()

#ifndef IN_LIBGCC2
#ifndef RETURN
#ifndef AND
#ifndef GENERATOR_FILE
#include <stdlib.h>
#include <safe-ctype.h>
#include "system.h"
#include "bitmap.h"
#define fancy_abort fancy_abort_rtl
#include "hard-reg-set.h"
#include "rtl.h"
#include "real.h"
#define DEFTREECODE(SYM, STRING, TYPE, NARGS)   XAP##SYM,
enum xap_tree_code {
#include "tree.def"
XAP_LAST_AND_UNUSED_TREE_CODE};
#undef DEFTREECODE
#define lhd_tree_inlining_walk_subtrees xap_tree_inlining_walk_subtrees
#define lhd_tree_size xap_tree_size
#define walk_tree_fn void*
#define tree_code xap_tree_code
typedef enum tree_code tree_code_t;
#include "langhooks-def.h"
#undef walk_tree_fn
#undef tree_code
#undef lhd_tree_size
#undef lhd_tree_inlining_walk_subtrees

typedef void (*switch_t(FILE *, int, int, const char *, const char *, const char *));
extern void override_options(switch_t p);

/* Names to predefine in the preprocessor for this target machine.  */

typedef void (*builtin_define_t(void *, const char *));
void xap2plus_cpu_cpp_builtins(void *, builtin_define_t);
#define TARGET_CPU_CPP_BUILTINS() xap2plus_cpu_cpp_builtins((void *)pfile, cpp_define)

typedef struct {int flag;
                rtx value1, value2;
                rtx ld_hard_regs[FIRST_PSEUDO_REGISTER];
                rtx st_hard_regs[FIRST_PSEUDO_REGISTER];
                } cc_status_mdep;

#undef LANG_HOOKS_EXPAND_CONSTANT
#define LANG_HOOKS_EXPAND_CONSTANT xap_lang_hooks_expand_constant
#undef LANG_HOOKS_FUNCTION_INIT
#define LANG_HOOKS_FUNCTION_INIT xap_lang_hooks_function_init
#undef LANG_HOOKS_FUNCTION_FINAL
#define LANG_HOOKS_FUNCTION_FINAL xap_lang_hooks_function_final
#undef LANG_HOOKS_TREE_INLINING_WALK_SUBTREES
#define LANG_HOOKS_TREE_INLINING_WALK_SUBTREES xap_tree_inlining_walk_subtrees
#undef LANG_HOOKS_TREE_SIZE
#define LANG_HOOKS_TREE_SIZE xap_tree_size

extern tree xap_lang_hooks_expand_constant(tree);
extern void xap_lang_hooks_function_init(struct function *f);
extern void xap_lang_hooks_function_final(struct function *f);

#undef fancy_abort
#undef abort

extern void print_operand(FILE *file, rtx addr, int letter);
extern void output_addr_vec_elt(FILE *file, int VALUE);
extern void output_case_label(FILE *file, const char *l, int number, void *insn);
extern void output_addr_diff_elt(FILE *file, void *body, int VALUE1, int VALUE2);
extern void output_ascii(FILE *file, const char *s, int len);
extern rtx function_arg(int cum, enum machine_mode mode, tree type, int named);
extern int xap_function_arg_partial_nregs(int cum, enum machine_mode mode, tree type, int named);
extern int xap2plus_return_in_memory(tree type);
extern int default_rtx_costs(rtx x, int outer_code);
extern int asm_output_special_pool_entry(FILE *file, rtx x, int mode, int align, int labelno);

enum reg_class xap_secondary_input_reload_class(enum reg_class CLASS, enum machine_mode MODE, rtx IN);
enum reg_class xap_secondary_output_reload_class(enum reg_class CLASS, enum machine_mode MODE, rtx IN);
int xap_secondary_memory_needed(enum reg_class CLASS1, enum reg_class CLASS2, enum machine_mode MODE);
rtx xap_secondary_memory_needed_rtx(enum machine_mode MODE);

/* Skip is used to step past uninitalised space in structs */

void asm_output_skip(FILE *f, int siz);

#define ASM_OUTPUT_SKIP(FILE,SIZE) asm_output_skip(FILE,SIZE)

extern void xap_file_start(void);
extern void xap_file_end(void);
extern void asm_identify_gcc(FILE *file);

extern void final_prescan_insn(rtx insn, rtx *operands, int n, int line, const char *file);

#if 0
#undef FINAL_PRESCAN_LABEL
#endif
#define FINAL_PRESCAN_INSN(insn, op, n) final_prescan_insn(insn, op, n, last_linenum, last_filename)

int legitimate_address(enum machine_mode mode, rtx x, int strict);
int reg_ok_for_base_p(rtx x, int strict);

/* Try machine-dependent ways of modifying an illegitimate address
   to be legitimate.  If we find one, return the new, valid address.
   This macro is used in only one place: `memory_address' in explow.c.

   OLDX is the address as it was before break_out_memory_refs was called.
   In some cases it is useful to look at this to decide what needs to be done.

   MODE and WIN are passed so that this macro can use
   GO_IF_LEGITIMATE_ADDRESS.

   It is always safe for this macro to do nothing.  It exists to recognize
   opportunities to optimize the output.  */

int legitimize_address(rtx x, rtx oldx, enum machine_mode mode);

/* This is how to output an assembler line
   that says to advance the location counter
   to a multiple of 2**LOG bytes. 
*/

extern void asm_output_align(FILE *file, int log);

#define ASM_OUTPUT_ALIGN(FILE,LOG) asm_output_align(FILE, LOG)

extern void asm_output_labelref(FILE *file, const char *name);

#define ASM_OUTPUT_LABELREF(file, name) asm_output_labelref(file, name)

/* This is how to output the definition of a user-level label named NAME,
   such as the label on a static function or variable NAME.  */

extern void asm_output_label(FILE *file, const char *name);

#define ASM_OUTPUT_LABEL(FILE,NAME) asm_output_label(FILE, NAME)

/* This is how to output a command to make the user-level label named NAME
   defined for reference from other files.  */

extern void asm_globalize_label(FILE *file, const char *name);
extern void asm_output_external(FILE *file, tree decl, const char *name);
extern void asm_output_external_libcall(FILE *file,rtx symref);
extern void asm_declare_object_name(FILE *file, const char *name, tree decl);

#define ASM_OUTPUT_EXTERNAL(FILE,DECL,NAME) asm_output_external(FILE,DECL,NAME)

#define ASM_OUTPUT_EXTERNAL_LIBCALL(FILE,SYMREF) asm_output_external_libcall(FILE,SYMREF)

#define ASM_DECLARE_OBJECT_NAME(FILE,NAME,DECL) asm_declare_object_name(FILE,NAME,DECL)

/* The prefix to add to user-visible assembler symbols. */

#define USER_LABEL_PREFIX ""

/* This is how to output an internal numbered label where
   PREFIX is the class of label and NUM is the number within the class.  */

extern void asm_output_internal_label(FILE *file, const char *prefix, int num);

#if 0
#define ASM_OUTPUT_INTERNAL_LABEL(FILE,PREFIX,NUM) \
    asm_output_internal_label(FILE, PREFIX, NUM)
#endif

/* This says how to output an assembler line
   to define a global common symbol.  */

extern void asm_output_common(FILE *file, const char *name, int size, int rounded);
extern void asm_output_local(FILE *file, const char *name, int size, int rounded);

#define ASM_OUTPUT_COMMON(FILE, NAME, SIZE, ROUNDED) asm_output_common(FILE, NAME, SIZE, ROUNDED)

#define ASM_OUTPUT_BSS(stream, decl, name, size, rounded) \
        asm_output_bss(stream,decl,name,size,rounded)

/* This says how to output an assembler line
   to define a local common symbol.  */

#define ASM_OUTPUT_LOCAL(FILE, NAME, SIZE, ROUNDED) asm_output_local(FILE, NAME, SIZE, ROUNDED)

extern void asm_finish_declare_object(FILE *file, tree d, int top, int end);

#define ASM_FINISH_DECLARE_OBJECT(FILE, DECL, TOP, END) \
  asm_finish_declare_object (FILE, DECL, TOP, END)

extern void asm_output_pool_prologue(const char *name, tree d, int off);

#define ASM_OUTPUT_POOL_PROLOGUE(asm_out_file, fnname, fndecl, pool_offset) \
  asm_output_pool_prologue (fnname, fndecl, pool_offset)

extern void asm_output_pool_epilogue(const char *name, tree d, int off);

#define ASM_OUTPUT_POOL_EPILOGUE(asm_out_file, fnname, fndecl, pool_offset) \
  asm_output_pool_epilogue (fnname, fndecl, pool_offset)

extern tree decl,type;
extern enum machine_mode xap_pmode(tree decl, tree type, const char *, int);
extern int xap_pointer_size(tree decl, tree type, const char *, int);
extern void retry_global_alloc(int, HARD_REG_SET);
extern int class_likely_spilled(enum reg_class class);
extern int hard_register_operand(rtx op, enum machine_mode mode);
extern int reload_operand(rtx op, enum machine_mode mode);
extern int reload_in_operand(rtx op, enum machine_mode mode);
extern int reload_out_operand(rtx op, enum machine_mode mode);
extern int xap2plus_expand_arith_hi3(RTX_CODE operator, rtx *operands);
extern int xap2plus_expand_div_qi3(RTX_CODE operator, RTX_CODE extend_operator, rtx *operands);
extern int xap2plus_expand_hi3(RTX_CODE operator, rtx *operands);
extern int xap2plus_expand_movhi(rtx *operands);
extern int xap2plus_expand_movqi(rtx *operands);
extern int xap2plus_expand_movstrqi(rtx *operands);
extern int xap2plus_expand_mult_qi3(RTX_CODE operator, RTX_CODE extend_operator, rtx *operands);
extern int xap2plus_expand_plus_hi3(RTX_CODE operator, rtx *operands);
extern int xap2plus_expand_qi3(RTX_CODE operator, rtx *operands);
extern int xap2plus_expand_reload_inqi(rtx *operands);
extern int xap2plus_expand_reload_outqi(rtx *operands);
extern int xap2plus_expand_reload_inhi(rtx *operands);
extern int xap2plus_expand_reload_outhi(rtx *operands);
extern int xap2plus_expand_shift_qi3(RTX_CODE operator, rtx *operands);
extern int xap2plus_expand_shift_hi3(RTX_CODE operator, rtx *operands);
extern int xap2plus_expand_compare_reg (int relational, void *destination);
extern int xap2plus_expand_cmpqi(rtx *operands);
extern int xap2plus_expand_cmphi(rtx *operands);
extern int register_move_cost(enum machine_mode MODE, enum reg_class CLASS1, enum reg_class CLASS2);
extern int memory_move_cost(enum machine_mode MODE, enum reg_class CLASS, int DIRECTION);

extern const char *xap2_plus_call (rtx *operands);

extern int reg_set_or_referenced_p (rtx reg, rtx from_insn);
extern void notice_update_cc (void);
extern void print_operand_address(FILE *f, rtx addr);

extern const char *module_file_name(void);
extern void output_double_xap(FILE *file, REAL_VALUE_TYPE value);
extern void output_float_xap(FILE *file, REAL_VALUE_TYPE value);
extern void output_short_float_xap(FILE *file, REAL_VALUE_TYPE value);
extern int xap_was_nz(void);

extern enum reg_class preferred_reload_class(rtx X, enum reg_class class);

extern void init_once_reg_set(void);

int xap2_load_cache(void *rtx1, void *rtx2);

#ifdef  __NO_STRING_INLINES
#define GCC_3P4
#else
#define GCC_3P3
#endif

#ifdef GCC_3P4
#define ASM_OUTPUT_SOURCE_LINE(file, lineno, counter) xap2_plus_output_source_line(lineno, xap_file)
#else
#define ASM_OUTPUT_SOURCE_LINE(file, lineno) xap2_plus_output_source_line(lineno, xap_file)
#endif

#ifdef GCC_3P4
#define INIT_CUMULATIVE_ARGS(CUM,FNTYPE,LIBNAME,FNDECL,INDIRECT) ((CUM) = 0)
#else
#define INIT_CUMULATIVE_ARGS(CUM,FNTYPE,LIBNAME,INDIRECT) ((CUM) = 0)
#endif

#ifdef GCC_3P4
extern void register_target_pragmas(void);
#else
extern void register_target_pragmas(void *);
#define ASM_OUTPUT_INTERNAL_LABEL(FILE,PREFIX,NUM) asm_output_internal_label(FILE, PREFIX, NUM)
#define ASM_FILE_START(FILE) xap_file_start()
#define ASM_FILE_END(FILE) xap_file_end()
#define MACHINE_DEPENDENT_REORG(insns) machine_dep_reorg(insns)
#define DEFAULT_RTX_COSTS(X,CODE,OUTER_CODE) \
    total = default_rtx_costs(X,OUTER_CODE); \
    if (total == -1) total = 0; else return total;
#define ADDRESS_COST(ADDR) 0
#define INIT_TARGET_OPTABS init_target_optabs()
#endif


#undef BITMAP_INIT_ONCE
#define BITMAP_INIT_ONCE() init_once_reg_set()

#endif
#endif
#endif
#endif

#define PUSH_ARGS_REVERSED XAP_FLAG_ENABLED(PUSH_REVERSED)

extern void init_target_optabs(void);

tree round_type_size(tree type, tree unpadded_size, int align);

#if 0
#define ROUND_TYPE_SIZE(TYPE, COMPUTED, SPECIFIED) round_type_size(TYPE, COMPUTED, SPECIFIED)
#define ROUND_TYPE_SIZE_UNIT(TYPE, COMPUTED, SPECIFIED) round_type_size(TYPE, COMPUTED, SPECIFIED)
#endif

int round_type_align(tree type, int align, int bits);

#define ROUND_TYPE_ALIGN(TYPE, ALIGN, BITS) round_type_align(TYPE, ALIGN, BITS)

#ifdef DEFAULT_TARGET_MACHINE
#undef input_filename
#endif

#endif
