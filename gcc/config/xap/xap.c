/*
 * Subroutines for insn-output.c for GNU compiler, for the Xap
 * JBS 25 Oct 2000, based on JRRK's XAP description
 */

/* Warning: these includes are really fragile */

#include "config.h"
#include "system.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <search.h>
#include <safe-ctype.h>
#include <ctype.h>


#ifdef __NO_STRING_INLINES
# define GCC_3P4
# include "coretypes.h"
# include "tm.h"
#else
# define GCC_3P3
#endif

#include "input.h"
#include "tree.h"
#include "local-xap.h"
#include "toplev.h"
#include "tm_p.h"
#include "rtl.h"
#include "regs.h"
#include "hard-reg-set.h"
#include "real.h"
#include "insn-config.h"
#include "conditions.h"
#include "function.h"
#include "output.h"
#include "insn-attr.h"
#include "flags.h"
#include "except.h"
#include "recog.h"
#include "expr.h"
#include "c-tree.h"
#include "ggc.h"
#include "c-pragma.h"
#include "basic-block.h"
#include "langhooks.h"

#ifdef GCC_3P4
# include "insn-modes.h"
#endif

int xap_target_default = 
    XAP_FLAG_MASK(TEST4)          + XAP_FLAG_MASK(LEAF_OPTIM) +
    XAP_FLAG_MASK(BITFIELD_OPT)   + XAP_FLAG_MASK(LOAD_CACHE) +
    XAP_FLAG_MASK(ENHANCED)       + XAP_FLAG_MASK(BLOCK_COPY) +
    XAP_FLAG_MASK(QUIET)          + XAP_FLAG_MASK(PEEPHOLE);

#define  function_overhead ((XAP_FLAG_ENABLED(LARGE))?2:1)
int asm_file_started = 0;
static int functions_used = 0;
static int xap_object_size = 0;
static int xap_excess_size = 0;
static int xap_expected_size = 0;
static int xap_assembled_integer = 0;
static unsigned int xap_thissize = 0;
static bool output_jtbase_label = false;

typedef struct lif {
    const char *name;
    struct lif *chain;
    tree node;
    int function:1;
    int global:1;
    int function_addressof:1;
    int import:1;
    int export:1;
    int referenced:1;
    int local_func:1;
    int declared:1;
} lif_t;

static lif_t *head = NULL;

typedef enum { LIF_SEARCH_FIND, LIF_SEARCH_ADD } lif_search_action;
static lif_t *lif_search(const char *name, lif_search_action);

static void asm_output_xapasm_fmt(FILE *file, const char *name, int global);
static void assemble_ident(FILE *file, tree node);
static void debug_lif(FILE *file, lif_t *ptr);

#ifndef REAL_ARITHMETIC
void real_value_to_target_double(void *r, long OUT[])
{
    REAL_VALUE_TYPE *IN = (REAL_VALUE_TYPE *)r;
    union {
        REAL_VALUE_TYPE f;
        HOST_WIDE_INT l[2];
    } u;
    if (sizeof(HOST_WIDE_INT) * 2 < sizeof(REAL_VALUE_TYPE))
        abort ();
    u.l[0] = u.l[1] = 0;
    u.f = *(IN);
    if (HOST_FLOAT_WORDS_BIG_ENDIAN == FLOAT_WORDS_BIG_ENDIAN)
        (OUT)[0] = u.l[0], (OUT)[1] = u.l[1];
    else
        (OUT)[1] = u.l[0], (OUT)[0] = u.l[1];
    xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT) && XAP_FLAG_ENABLED(DOUBLE));
}

long real_value_to_target_single(void *r)
{
    REAL_VALUE_TYPE *IN = (REAL_VALUE_TYPE *)r;
    long OUT;
    union {
        float f;
        HOST_WIDE_INT l;
    } u;
    if (sizeof(HOST_WIDE_INT) < sizeof(float))
        abort ();
    u.l = 0;
    u.f = *(IN);
    (OUT) = u.l;
    xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT));
    return OUT;
}
#endif

int can_eliminate PARAMS((int from ATTRIBUTE_UNUSED, int to ATTRIBUTE_UNUSED))
{
    switch(from)
    {
        case RAP:
            return 1;
        case RSP:
            return 0;
        default:
            return 0;
    }
}

int initial_elimination_offset(int FROM, int TO)
{
    int OFFSET = 0;
    if (TO != RFP) abort();
    switch(FROM)
    {
        case RAP:
            /* In TARGET_LEAF_OPTIM mode we may avoid having any function
             * overhead */
            if(   XAP_FLAG_ENABLED(LEAF_OPTIM)
               && current_function_is_leaf
               && !get_frame_size()
               && (!XAP_FLAG_ENABLED(LARGE) || !regs_ever_live[RXH])
               && !regs_ever_live[RXL] )
            {
                OFFSET = 0;
            }
            else
            {
                OFFSET =   function_overhead
                         + get_frame_size()
                         + current_function_outgoing_args_size;
            }
            break;
        case RSP:
            OFFSET = 0;
            break;
        default:
            OFFSET = 0;
    }
    return OFFSET;
}

static const char * const reg_class_names[] = REG_CLASS_NAMES;

static char base_name[256];
static int first_function = 0;
static switch_t *print_switch_values_ptr;
static int could_be_case_label_diff = 0;
static int pagezero = 0;

static char *module_base_name(void)
{
    return base_name;
}

#undef RCS_DATE
#define RCS_DATE "$Date: 2004/05/17 08:12:23 $"
#undef RCS_NAME
#define RCS_NAME "$Name:  $"

void xap_adjust_type(FILE *file, tree node);
void xap_adjust_record(FILE *file, tree node);

typedef struct rli_chain
{
    struct rli_chain *nxt;
    tree rli;
} *rli_chain_t;

static rli_chain_t rli_chain_head = NULL;

static void xap_adjust_rli(record_layout_info rli)
{
    if (!current_function_decl)
    {
        rli_chain_t nxt = xmalloc(sizeof(rli_chain_t));
        nxt->nxt = rli_chain_head;
        nxt->rli = rli->t;
        rli_chain_head = nxt;
    }
}

static void xap_dump_rli(void)
{
    rli_chain_t nxt = rli_chain_head;
    while (nxt)
    {
        if (nxt->rli) xap_adjust_type(asm_out_file, nxt->rli);
        nxt = nxt->nxt;
    }
    rli_chain_head = NULL;
}

void xap2_plus_first_function(void)
{
    if (first_function)
    {
        rtx x;
        char *s;
        char *d;
        const char * const base = XAP_FLAG_ENABLED(MANGLED_MODULE_NAMES)
                                    ? aux_base_name : dump_base_name;

        if (*base_name) strcat(base_name, "_");
        else if (isdigit(*base)) strcat(base_name, "gcc_");
        strcat(base_name, base);

        d = strrchr(base_name,'.');  
        s = strrchr(base_name,'/');  
        if(!s) s = strrchr(base_name,'\\');
        if (d && s < d) *d = 0;

        if(XAP_FLAG_ENABLED(MANGLED_MODULE_NAMES))
        {
            if (first_global_object_name)
            {
                strcat(base_name, "_");
                strcat(base_name, first_global_object_name);
            }
            if (current_function_decl)
            {
                x = DECL_RTL(current_function_decl);
                if (GET_CODE (x) != MEM)
                    abort ();
                x = XEXP (x, 0);
                if (GET_CODE (x) != SYMBOL_REF)
                    abort ();
                strcat(base_name, "_");
                strcat(base_name, XSTR (x, 0));
            }
            else
            {
                strcat(base_name, "_default");
            }
        }
        for(s = base_name; *s; ++s)
            if(!isalnum((int)*s)) *s = '_';
        first_function = 0;
        fprintf(asm_out_file, "\tMODULE\t%s\n\t.%s\n", 
                module_base_name(),
                XAP_FLAG_ENABLED(LARGE) ? "LARGE"
               : XAP_FLAG_ENABLED(XAP2) ? "SMALL"
               : "LARGE ; really small mode but we need space for debug symbols"
               );
        fprintf (asm_out_file, "; GNU C version %s", version_string);
        fnotice (asm_out_file, 
#ifdef __GNUC__
                "\n; \tcompiled by GNU C version %s.\n"
#else
                " compiled by CC.\n"
#endif
                , __VERSION__);
        /* Print the list of options in effect.  */
        if (print_switch_values_ptr)
            print_switch_values_ptr (
                             asm_out_file, 0, 75, asm_comment_start(), " ", "\n"
                                    );
        /* Add a blank line here so it appears in assembler output but not
           screen output.  */
        fprintf (asm_out_file, "\n");
        if (XAP_FLAG_ENABLED(ENHANCED))
        {
            fprintf (asm_out_file, "\t.ENHANCED\n");
        }
        else fprintf (asm_out_file, "\n");
    }
}

const char *asm_app_on(void)
{
    xap2_plus_first_function();
    return ";\n; inline assembly begins\n;\n";
}

const char *asm_app_off(void)
{
    xap2_plus_first_function();
    return ";\n; inline assembly ends\n;\n";
}

const char *text_section_asm_op(void)
{
    xap2_plus_first_function();
    return  "\t.CODE";
}

const char *data_section_asm_op(void)
{
    xap2_plus_first_function();
    return "\t.SEG\tINIT";
}

const char *initc_section_asm_op(void)
{
    xap2_plus_first_function();
    return "\t.SEG\tINITC";
}

const char *bss_section_asm_op(void)
{
    xap2_plus_first_function();
    return  (pagezero ? "\t.SEG\tZVAR" : "\t.SEG\tVAR") ;
}

const char *const_section_asm_op(void)
{
    xap2_plus_first_function();
    return  "\t.SEG\tCONST" ;
}

const char *xconst_section_asm_op(void)
{
    xap2_plus_first_function();
    return  "\t.SEG\tXCONST" ;
}

const char *asm_comment_start(void)
{
    return  ";#";
}

static void print_rtl_xap(rtx addr)
{
            print_rtx_head = asm_comment_start();
            print_rtl_single (asm_out_file, addr);
            print_rtx_head = "";      
}

rtx xap_crnt_pattern;
rtx xap_penultimate_pattern;

void xap_test_optim(rtx *operands)
{
int test_deleted = 0;
rtx exp = xap_penultimate_pattern ? xap_penultimate_pattern : NULL;
rtx dest = (exp && GET_CODE(exp)==SET) ? SET_DEST(exp) : NULL;
rtx src = (exp && GET_CODE(exp)==SET) ? SET_SRC(exp) : NULL;
if (exp && XAP_FLAG_ENABLED(TEST_OPTIM) && (GET_CODE(exp)==SET)) switch(GET_CODE(src))
   {
   case MEM:
        if ((GET_CODE(operands[0])==MEM) 
            && rtx_equal_p(dest, gen_rtx_REG(QImode, RXL))
            && rtx_equal_p(src,operands[0]))
                {
                if (!quiet_flag)
                    {
                    fprintf(asm_out_file, ";# testing memory with X modified - unsafe to remove\n");
                    print_rtl_xap (src);
                    print_rtl_xap (operands[0]);
                    }
                break;
                }
       /* else drop through */
   case CONST:
   case CONST_INT:
   case CONST_STRING:
   case REG:
        if (rtx_equal_p(src,operands[0])
             && !(GET_CODE(src)==REG && REGNO(src)>RSPECIAL && ((GET_CODE(dest)==REG && REGNO(dest)>RSPECIAL) || GET_CODE(dest)==MEM)))
            {
            if (!quiet_flag)
                {
                fprintf(asm_out_file, ";# test removed ");
                fprintf(asm_out_file, "due to source of previous insn matching test\n");
                print_rtl_xap (src);
                }
            test_deleted = 1;
            }
       /* drop through */
   case AND:
   case IOR:
   case XOR:
   case PLUS:
   case MINUS:
        if (rtx_equal_p(dest,operands[0]))
            {
            if (!quiet_flag)
                {
                fprintf(asm_out_file, ";# test removed ");
                fprintf(asm_out_file, "due to destination of previous insn matching test\n");
                print_rtl_xap (dest);
                }
            test_deleted = 1;
            }
      break;
   case ASHIFT:
   case ASHIFTRT:
   case LSHIFTRT:
   case CALL:
   case SUBREG:
   case COMPARE:
   case NEG:
   case NOT:
   case MULT:
   case DIV:
   case MOD:
   case UDIV:
   case UMOD:
   case ROTATE:
   case ROTATERT:
   case SIGN_EXTEND:
   case ZERO_EXTEND:
   default:
        break;
   }
if (XAP_FLAG_ENABLED(TEST_OPTIM) && test_deleted)
   {
      fputc(';', asm_out_file);
   }
}

static void xap_referenced_fun(rtx addr, FILE *file)
{
    switch(GET_CODE(addr))
    {
        case COMPARE:
        case PLUS:
        case MINUS:
        case AND:
        case IOR:
        case XOR:
        case MULT:
        case DIV:
        case MOD:
        case UDIV:
        case UMOD:
        case ASHIFT:
        case LSHIFTRT:
        case ASHIFTRT:
        case SET:
            xap_referenced_fun(XEXP(addr, 0), file);
            xap_referenced_fun(XEXP(addr, 1), file);
            break;
        case LABEL_REF:
        case SUBREG:
        case REG:
        case SCRATCH:
        case IF_THEN_ELSE:
        case ADDR_DIFF_VEC:
        case CC0:
        case PC:
        case ASM_INPUT:
            break;
        case CALL:
        case MEM:
        case CONST:
        case SIGN_EXTEND:
        case ZERO_EXTEND:
        case TRUNCATE:
        case NEG:
        case NOT:
            xap_referenced_fun(XEXP(addr, 0), file);
            break;
        case CONST_INT:
            break;
        case SYMBOL_REF:
            {
                const char *name = XSTR(addr,0);
                if (name[0] == '*')
                    fprintf(
                           file, "%s import %s\n", asm_comment_start(), &name[1]
                           );
                else
                {
                    lif_t *ptr = lif_search(name, LIF_SEARCH_ADD);
                    if (!ptr) abort();
                    if (!TREE_ASM_WRITTEN(ptr->node) && !ptr->declared)
                    {
                        ptr->declared = 1;
                        fprintf(file, "%s import ", asm_comment_start());
                        asm_output_xapasm_fmt (file, name, ptr->global);
                        fprintf(file, "\n");
                    }
                }
                break;
            }
        default:
            print_rtl_xap(addr);
            break;        
    }
}

void machine_dep_reorg(void *insns ATTRIBUTE_UNUSED)
{
    FILE *file = asm_out_file;

    xap2_plus_first_function();

    if (! functions_used)
    {
        if (XAP_FLAG_ENABLED(FRAME_OPTIM))
        {
            fprintf (
                        file,
                        "\tIFDEF\t$GNU_DEBUG\n"
                        "\tELSE\n"
                        "prologue\tMACRO\toff\n"
                        "\tenterl\t&off\n"
                        "\tENDMAC\n\tENDIF\n"
                    );
            fprintf (
                        file,
                        "\tIFDEF\t$GNU_DEBUG\n"
                        "\tELSE\n"
                        "epilogue\tMACRO\toff\n"
                        "\tleavel\t&off\n"
                        "\tENDMAC\n\tENDIF\n"
                    );
        }
    }

    functions_used = 1;
}

enum {max_alternatives = 10};

typedef struct insn_stats
{
    int format;
    int count_single;
    int count_multi[max_alternatives];
    int count_function[max_alternatives];
} insn_stats_t;

static unsigned int max_lines;
static int max_codes, max_code_used;
static insn_stats_t *count_codes;
static unsigned short *count;
static unsigned int xap_last_linenum;
char xap_cwd[2048];
char xap_file[2048];
static bool xap_file_changed = true;

void xap_set_cwd(const char *cwd)
{
    if(strlen(cwd) >= sizeof(xap_cwd))
        abort();
    strcpy(xap_cwd, cwd);
}

void xap_set_file(const char *file)
{
    if(strlen(file) >= sizeof(xap_file))
        abort();
    if (!strcmp(xap_file,file))
        xap_file_changed = true;
    strcpy(xap_file, file);
}

static char nibble(int c)
{ return "0123456789abcdef"[c & 15]; }

static void convert_to_hex(const char *s, char *t)
{
    while(*s)
    {
        *t++ = nibble(*s >> 4);
        *t++ = nibble(*s);
        ++s;
    }
    *t = '\0';
}

int reg_class_from_letter(int c)
{
    switch(c)
    {
        case 'a': return ALL_REGS;
        case 'd': return DATA_REGS;
        case 'y': return FRAME_REGS;
        case 'c': return CHIP_REGS;
        case 'f': return FAKE_REGS;
        case 'x': return ADDR_REGS;
        default : return ALL_REGS;
    }
    return NO_REGS;
}

int xap_const_ok_for_letter_p(int VALUE, int C)
{
    return
        ((C)=='I' ? -128 <= (VALUE) && (VALUE) <= 127 :
         (C)=='J' ? -32767 <= (VALUE) && (VALUE) <= 32767 :
         (C)=='K' ? VALUE == 0 || reload_completed:
         0);
}

static void print_register_name(FILE *file, int regno, int letter)
{
    if (regno < FIRST_PSEUDO_REGISTER)
    {
        if ((letter == 'f') && (regno < 8))
        {
            rtx x = xap2_plus_reg_rtx(regno);
            print_operand(file, x, letter);
        }
        else fprintf(file, "%s", reg_names[regno]);
    }
    else
        fprintf(file, "R%d", regno);
}

static void asm_output_symbolref(FILE *file, const char *name)
{
    lif_t *ptr = lif_search(name, LIF_SEARCH_FIND);
    if (ptr && (*name != '*'))
    {
        ptr->referenced = 1;
        {
            asm_output_xapasm_fmt (file, name, ptr->global);
        }
    }
    else
        assemble_ident(file, get_identifier(name));
}

static void print_memory(FILE *file, rtx addr, int offset, int letter)
{
    /* Matches the shape if GO_IF_LEGITIMATE_ADDRESS */
    switch(GET_CODE(addr))
    {
        case REG:
            print_register_name(file, REGNO(addr) + offset, letter);
            break;
        case SUBREG:
            print_register_name(
                                   file,
                                   REGNO(XEXP(addr,0)) + XINT(addr, 1) + offset,
                                   letter
                               );
            break;
        case PLUS:
            {
                rtx lnk0 = XEXP(addr,0);
                rtx lnk1 = XEXP(addr,1);
                switch(GET_CODE(lnk0))
                {
                    case REG:
                        fprintf(file, "(");
                        print_memory(file, lnk1, offset, letter);
                        fprintf(file, ",");
                        print_register_name(file, REGNO(lnk0), ' ');
                        fprintf(file, ")");
                        break;
                    case SYMBOL_REF:
                        asm_output_symbolref(file, XSTR(lnk0,0));
                        fprintf(file, "+");
                        print_memory(file, lnk1, offset, letter);
                        break;
                    default:
                        print_memory(file, lnk0, offset, letter);
                        if (GET_CODE(lnk1)==CONST_INT)
                        {
                            int off = INTVAL(lnk1);
                            if (off < 0)
                                fprintf(file, "-H'%.4X", -off);
                            else
                                fprintf(file, "+H'%.4X", off);
                        }
                        else
                        {
                            fprintf(file, " + ");
                            print_memory(file, lnk1, 0, letter);
                        }
                        break;
                }
                break;
            }
        case CONST:
            {
                rtx lnk2 = XEXP(addr,0);
                switch(GET_CODE(lnk2))
                {
                    case PLUS:
                        {
                            rtx lnk3 = XEXP(lnk2,0);
                            rtx lnk4 = XEXP(lnk2,1);
                            switch(GET_CODE(lnk4))
                            {
                                case CONST_INT:
                                    {
                                        int off = INTVAL(lnk4)+offset;
                                        asm_output_symbolref(
                                                                file, 
                                                                XSTR(lnk3,0)
                                                            );
                                        if (off < 0)
                                            fprintf(file, "-H'%.4X", -off);
                                        else
                                            fprintf(file, "+H'%.4X", off);
                                        break;
                                    }
                                default:
                                    asm_output_symbolref(file, XSTR(lnk3,0));
                                    fprintf(file, "+");
                                    print_memory(file, lnk4, offset, letter);
                                    break;
                            }
                        }
                        break;
                    case MINUS:
                        {
                            rtx lnk3 = XEXP(lnk2,0);
                            rtx lnk4 = XEXP(lnk2,1);
                            print_memory(file, lnk3, offset, letter);
                            fprintf(file, "-");
                            print_memory(file, lnk4, offset, letter);
                        }
                        break;
                    default:
                        print_memory(file, XEXP(addr, 0), offset, letter);
                        abort();
                        break;
                }
            }
            break;
        case CONST_INT:
            {
                int off = INTVAL(addr)+offset;
                if (off < 0)
                    fprintf(file, "-H'%.4X", -off);
                else
                    fprintf(file, "H'%.4X", off);
            }
            break;
        case SYMBOL_REF:
            if(offset) 
            {
                fprintf(file, "(");
                asm_output_symbolref(file, XSTR(addr,0));
                fprintf(file, "+%d)", offset);
            }
            else
                asm_output_symbolref(file, XSTR(addr,0));
            break;
        case LABEL_REF:
            print_memory(file, XEXP(addr, 0), offset, letter);
            break;
        case CODE_LABEL:
            if(letter=='j') output_jtbase_label = true;
            output_addr_const(file, addr);
            if(offset) fprintf(file, "+1");
            output_jtbase_label = false;
            break;        
        case MEM:
            print_operand(file, addr, ' ');
            break;
        case SCRATCH:
            fprintf(file, "@$scratch");
            abort();
            break;
        case CONST_DOUBLE:
            xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT) && XAP_FLAG_ENABLED(DOUBLE));
            fprintf(file,  "H'%.4X", (INTVAL(addr)&0xFFFF) + offset);
            break;
        case MINUS:
            if(GET_CODE(XEXP(addr,0))==REG)
            {
                fprintf(file, "(-");
                print_memory(file, XEXP(addr,1), offset, letter);
                fprintf(file, ",");
                print_register_name(file, REGNO(XEXP(addr,0)), letter);
                fprintf(file, ")");
            }
            else
            {
                could_be_case_label_diff = 1;
                print_memory(file, XEXP(addr, 0), offset, letter);
                fprintf(file, "-");
                print_memory(file, XEXP(addr, 1), 0, letter);
                could_be_case_label_diff = 0;
            }
            break;
        case CC0:
            fprintf(file, "flags");
            break;
        case NOTE:
            fprintf(file, "NOTE");	
            break;
        case MULT:
            fprintf(file, "(");
            print_memory(file, XEXP(addr, 0), offset, letter);
            fprintf(file, "*");
            print_memory(file, XEXP(addr, 1), 0, letter);
            fprintf(file, ")");
            break;
        case EQ:
            fprintf(file, "eq");
            break;
        case NE:
            fprintf(file,  "ne");
            break;
        case GT:
            fprintf(file,  "gt");
            break;
        case GTU:
            fprintf(file,  "gtu");
            break;
        case GE:
            fprintf(file,  "ge");
            break;
        case GEU:
            fprintf(file,  "cc");
            break;
        case LE:
            fprintf(file,  "le");
            break;
        case LEU:
            fprintf(file,  "cz");
            break;
        case LT:
            fprintf(file,  "lt");
            break;
        case LTU:
            fprintf(file,  "cs");
            break;
        case SIGN_EXTEND:
            fprintf(file,  "sext(");
            print_memory(file, XEXP(addr,0), offset, letter);
            fprintf(file,  ")");
            break;
        case ZERO_EXTEND:
            fprintf(file,  "zext(");
            print_memory(file, XEXP(addr,0), offset, letter);
            fprintf(file,  ")");
            break;
        default:
            debug_rtx(addr);
            abort();
            fprintf(file,"Invalid-switch-GET_CODE(addr)=%d\n", GET_CODE(addr));
            break;
    }
}

static int offset_from_letter(int letter)
{
    switch (letter)
    {
        case 'q': return -1;
        case 't': return 0;
        case 'b': return 1;
        case 'T': return 2;
        case 'B': return 3;
        default : return 0;
    }
}

void print_operand(FILE *file, rtx addr, int letter)
{
    if (!addr) fprintf(file, "(null)");
    else switch(GET_CODE(addr))
    {
        case SUBREG:
            fprintf(file, "subreg(");
            print_memory(
                    file, XEXP(addr, 0), offset_from_letter(letter), letter
                        );
            fprintf(file, ")");
            break;
        case REG:
        case SCRATCH:
            print_memory(file, addr, offset_from_letter(letter), letter);
            break;
        case MEM:
            {
                rtx lnk1 = XEXP(addr, 0);
                switch(GET_CODE(lnk1))
                {
                    case REG:
                        if(letter != 'c') fprintf(file, "@(");
                        fprintf(file, "%d,", offset_from_letter(letter));
                        print_register_name(file, REGNO(lnk1), ' ');
                        if(letter != 'c') fprintf(file, ")");
                        break;
                    case SYMBOL_REF:
                        {
                            int off1 = offset_from_letter(letter);
                            if (letter == 'c')
                            {
                                assemble_name(file, XSTR(XEXP(addr,0),0));
                            }
                            else if(off1) 
                            {
                                fprintf(file, "@");
                                asm_output_symbolref(file, XSTR(lnk1,0));
                                fprintf(file, "+%d", off1);
                            }
                            else
                            {
                                fprintf(file, "@");
                                asm_output_symbolref(file, XSTR(lnk1,0));
                            }
                        }
                        break;
                    case PLUS:
                        if(letter != 'c') fprintf(file, "@");
                        print_memory(
                                file, lnk1, offset_from_letter(letter), letter
                                    );
                        break;
                    case CONST:
                        if(letter != 'c') fprintf(file, "@");
                        print_memory(
                                file, lnk1, offset_from_letter(letter), letter
                                    );
                        break;
                    default:
                        if(letter != 'c') fprintf(file, "@");
                        print_memory(
                                file, lnk1, offset_from_letter(letter), letter
                                    );
                }
                break;
            }
        case CONST_INT:
            if ((letter != 'c') && (letter != 'C')) fprintf(file,  "#");
            switch(letter)
            {
                case 't': fprintf(file,  "H'%.4X", (INTVAL(addr)>>16)&0xFFFF);
                          break;
                case 'b': fprintf(file,  "H'%.4X", (INTVAL(addr))&0xFFFF);
                          break;
                default:  
                          print_memory(file, addr, 0, letter);
            }
            break;
        case PLUS:
            if ((letter != 'c') && (letter != 'C')) fprintf(file,  "#");
            fprintf(
                   file, letter == 't' ? "hwrd(" : letter == 'b' ? "lwrd(" : "("
                   );
            print_memory(file, addr, 0, letter);
            fprintf(file, letter == 't' || letter == 'b' ? ")" : ")");
            break;        
        case SYMBOL_REF:
            {
                lif_t *ptr = lif_search(XSTR(addr,0), LIF_SEARCH_FIND);
                if (ptr && ptr->function && !XAP_FLAG_ENABLED(BIG_FUNCTION_PTR))
                    ptr->function_addressof = 1;
                if ((letter != 'c') && (letter != 'C')) fprintf(file, "#");
                fprintf(
                            file,
                            letter == 't' ? "hwrd(" 
                          : letter == 'b' ? "lwrd(" 
                          : ptr && ptr->function ? "lwrd(" 
                                                 : "("
                       );
                asm_output_symbolref(file, XSTR(addr,0));
                fprintf(
                            file,
                            ptr && ptr->function
                            && XAP_FLAG_ENABLED(LARGE)
                            && !XAP_FLAG_ENABLED(BIG_FUNCTION_PTR) ? "\?\?)"
                                                        : ")"
                       );
            }
            break;        
        default:
            if ((letter != 'c') && (letter != 'C')) fprintf(file,  "#");
            if (letter != 'C')
                fprintf(
                   file, letter == 't' ? "hwrd(" : letter == 'b' ? "lwrd(" : "("
                       );
            print_memory(file, addr, 0, letter);
            if (letter != 'C')
                fprintf(file, letter == 't' || letter == 'b' ? ")" : ")");
            break;        
    }
}

void print_operand_address(FILE *file, rtx addr)
{
    fprintf(file, "@");
    print_memory(file, addr, 0, ' ');
}

/* used by ASM_OUTPUT_DOUBLE => DFmode, not valid on XAP */

void output_double_xap(
        FILE *file ATTRIBUTE_UNUSED, REAL_VALUE_TYPE value ATTRIBUTE_UNUSED
                      )
{
    xap2_plus_target_assert(0);
    abort();
}

/* used by ASM_OUTPUT_FLOAT => SFmode, double on XAP */

void output_float_xap(FILE *asm_out_file, REAL_VALUE_TYPE value)
{
    long l[2];
    REAL_VALUE_TO_TARGET_DOUBLE(value, l);
    xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT) && XAP_FLAG_ENABLED(DOUBLE));
    xap_object_size += 4;
    fprintf(
                asm_out_file,
                "\tdc\t0x%.4lX\n\tdc\t0x%.4lX\n\tdc\t0x%.4lX\n\tdc\t0x%.4lX\n", 
                (l[0]>>16)&0xFFFF, l[0]&0xFFFF, (l[1]>>16)&0xFFFF, l[1]&0xFFFF
           );
}

/* used by ASM_OUTPUT_SHORT_FLOAT => HFmode, float on XAP */

void output_short_float_xap(FILE *asm_out_file, REAL_VALUE_TYPE value)
{
    long l;
    REAL_VALUE_TO_TARGET_SINGLE(value, l);
    xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT));
    xap_object_size += 2;
    fprintf(asm_out_file, "\tdc\t0x%.4lX\n\tdc\t0x%.4lX\n",
            (l>>16)&0xFFFF, l&0xFFFF);

}

static void cc_status_mdep_all_init(int pos)
{
    if (!XAP_FLAG_ENABLED(QUIET))
        fprintf(asm_out_file, "; initializing all mdep - location %d\n\t", pos);

    CC_STATUS_INIT;
    cc_status.mdep.value1 = NULL;
    cc_status.mdep.value2 = NULL;
    memset(
            &cc_status.mdep.ld_hard_regs, 0, sizeof(cc_status.mdep.ld_hard_regs)
          );
    memset(
            &cc_status.mdep.st_hard_regs, 0, sizeof(cc_status.mdep.st_hard_regs)
          );
    memset(
  &cc_prev_status.mdep.ld_hard_regs, 0, sizeof(cc_prev_status.mdep.ld_hard_regs)
          );
    memset(
  &cc_prev_status.mdep.st_hard_regs, 0, sizeof(cc_prev_status.mdep.st_hard_regs)
          );
}

void cc_status_mdep_init(void)
{
}

void asm_output_internal_label(FILE *file, const char *prefix, int num)
{
    fprintf(file, "?%s%d:\n", prefix, num);
    cc_status_mdep_all_init(1);
    xap_crnt_pattern = NULL;
    xap_penultimate_pattern = NULL;
}

void asm_generate_internal_label(char *lab, const char *prefix, int num)
{
    if (could_be_case_label_diff && strcmp(prefix,"LC"))
        sprintf(lab,  "?%s%d", prefix, num);
    else if(output_jtbase_label)
        sprintf(lab,  "*?JTBASE%d", num);
    else
        sprintf(lab,  "*?%s%d", prefix, num);
}

void output_addr_vec_elt(FILE *file, int VALUE)
{
    xap_object_size += 1;
    if (XAP_FLAG_ENABLED(LARGE))
        fprintf (file, "\tdc\tlwrd(?L%d)\n", VALUE);
    else
        fprintf (file, "\tdc\t?L%d\n", VALUE);
}

void output_case_label(
              FILE *file, const char *l, int number, void *insn ATTRIBUTE_UNUSED
                      )
{
    if (XAP_FLAG_ENABLED(SWITCH_TABLE))
    {
        text_section();
        asm_output_internal_label(file, l, number);
    }
    else
    {
        asm_output_internal_label(file, "JTBASE", number);
        text_section();
        asm_output_internal_label(file, l, number);
        READONLY_DATA_SECTION();
    }
}

void output_addr_diff_elt(
                 FILE *file, void *body ATTRIBUTE_UNUSED, int VALUE1, int VALUE2
                         )
{
    if (XAP_FLAG_ENABLED(SWITCH_TABLE))
    {
        fprintf (file, "\tbra2\t?L%d\n", VALUE1);
    }
    else
    {
        xap_object_size += 1;
        if (XAP_FLAG_ENABLED(LARGE))
            fprintf (file, "\tdc\tlwrd(?L%d-?L%d)\n", VALUE1, VALUE2);
        else
            fprintf (file, "\tdc\t?L%d-?L%d\n", VALUE1, VALUE2);
    }
}

void output_ascii(FILE *file, const char *s, int len)
{
    if(!XAP_FLAG_ENABLED(QUIET))
        fprintf(file,"; output_ascii\n");

    while(--len >= 0)
    {
        char c = *s++;
        int n  = flag_signed_char ? (int)(signed char) c
                                  : (int)(unsigned char) c;
        xap_object_size += 1;
        fprintf(file,  "\tdc\tH'%.2X", n);
        fputc('\n', file);
    }
}

#ifdef HAVE_cc0
void notice_update_cc ()
{ CC_STATUS_INIT; }

int xap_was_nz(void)
{ return (cc_prev_status.flags & CC_NO_OVER_OR_CARRY) != 0; }
#endif

int reg_ok_for_base_p(rtx X, int strict)
{
    int ret;
    if (strict)
        ret = HARD_REGNO_OK_FOR_BASE_P(REGNO(X));
    else
        ret = (    REGNO(X)>=FIRST_PSEUDO_REGISTER
                || HARD_REGNO_OK_FOR_BASE_P(REGNO(X)) );
    return ret;
}

/* GO_IF_LEGITIMATE_ADDRESS recognizes an RTL expression
   that is a valid memory address for an instruction.
   The MODE argument is the machine mode for the MEM expression
   that wants to use this address.

*/

#define LEGITIMATE_BASE_REG_P(X, STRICT) \
( \
  ( GET_CODE(X)==REG \
    && reg_ok_for_base_p(X, STRICT) ) \
  || \
  ( GET_CODE(X)==SUBREG \
    && GET_CODE(SUBREG_REG(X))==REG \
    && reg_ok_for_base_p(SUBREG_REG(X), STRICT) ) \
)

int legitimize_address(
                        rtx X ATTRIBUTE_UNUSED, rtx OLDX ATTRIBUTE_UNUSED,
                        enum machine_mode mode ATTRIBUTE_UNUSED
                      )
{
    return 0;
}

int legitimate_address(
                      enum machine_mode mode ATTRIBUTE_UNUSED, rtx X, int strict
                      )
{
    if(LEGITIMATE_BASE_REG_P(X, strict))
        return 1;
    if(CONSTANT_ADDRESS_P(X))
        return 1;
    if(GET_CODE(X)==PLUS
            && GET_CODE(XEXP(X, 0))==REG
            && LEGITIMATE_BASE_REG_P(XEXP(X, 0), strict))
        switch(GET_CODE(XEXP(X,1)))
        {
            case CONST:
            case CONST_INT:
            case SYMBOL_REF:
            case LABEL_REF:
                return 1;
            default:
                break;
        }
    return 0;
}

int hard_regno_mode_ok(int regno, int mode)
{
    switch(mode)
    {
        case HImode: return (regno==RAH)||(regno>=8);
        case QImode: return reload_completed ? regno!=RXH : 1;
        case HFmode:
                     if(!first_function) xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT));
                     return (regno==8)||(regno==10);
        case SFmode:
                     if(!first_function) xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT)
                                                              && XAP_FLAG_ENABLED(DOUBLE));
                     return (regno==8);
        case SImode:
                     return (regno==8);
    }
    return 0;
}

int modes_tieable(int mode1, int mode2)
{
    return mode1 == mode2;
}

static tree node_to_ident(tree node, int *scope)
{
    int file_scope = 0;
#ifdef GCC_3P4
    tree node1 = IDENTIFIER_SYMBOL_VALUE (node);
    file_scope = DECL_FILE_SCOPE_P(node);
    if (node1) node = node1;
#else
    tree global = IDENTIFIER_GLOBAL_VALUE (node);
    tree local = IDENTIFIER_LOCAL_VALUE (node);
    if (global)
    {
        node = global;
        file_scope = 1;
    }
    if (local) node = local;
#endif
    if (scope) *scope = file_scope;
    return node;
}

static void assemble_ident(FILE *file, tree node)
{
    if (node && TREE_CODE(node)==TYPE_DECL)
        node = DECL_NAME(node);
    if (node && TREE_CODE(node)==IDENTIFIER_NODE)
    {
        const char *name = IDENTIFIER_POINTER(node);
        if (TREE_CODE_CLASS (TREE_CODE(node)) == 'x')
        {
            node = node_to_ident(node, NULL);
        }
        if ((TREE_CODE(node) == FUNCTION_DECL) && XAP_FLAG_ENABLED(LARGE))
        {
            lif_t *ptr = lif_search(name, LIF_SEARCH_FIND);
            ptr->referenced = 1;
            if (!XAP_FLAG_ENABLED(BIG_FUNCTION_PTR)) ptr->function_addressof = 1;
            if (ptr->global)
                fprintf (file,  "$%s", name);
            else
            {
                fprintf (file, "%s", name);
            }
        }
        else
            assemble_name(file, name);
    }
}

static void lif_dump1(void)
{
    int first = 1;
    lif_t *nxt = head;
    FILE *file = asm_out_file; 
    while (nxt)
    {
        const char *name = nxt->name;
        if (strncmp(name, "__builtin", 9) && nxt->function_addressof)
        {
            if (nxt->global || !XAP_FLAG_ENABLED(LARGE))
            {
                debug_lif (file, nxt);
            }
            else
            {
                if (first)
                    fprintf (file, "\t.CSEG\tLOWMEM\n");
                first = 0;
                fprintf (file, "%s??:\tbra3\t", name);
                asm_output_xapasm_fmt(file, name, nxt->global);
                fprintf (file, "\n");
            }
        }
        nxt = nxt->chain;
    }
}

static void lif_dump2(void)
{
    lif_t *nxt;
    FILE *file = asm_out_file;
    fprintf (file, "\n;LIF .DECLARED\n");
    nxt = head;
    while (nxt)
    {
        const char *name = nxt->name;
        if (strncmp(name, "__builtin", 9) && nxt->export)
        {
            fprintf (file, ";LIF ");
            asm_output_xapasm_fmt(file, name, nxt->global);
            fprintf (file, "\n");
        }
        nxt = nxt->chain;
    }
    fprintf (file, ";LIF .UNDECLARED\n");
    nxt = head;
    while (nxt)
    {
        const char *name = nxt->name;
        if (
                strncmp(name, "__builtin", 9)
             && nxt->import
             && nxt->referenced
             && !nxt->export
           )
        {
            fprintf (file, ";LIF ");
            asm_output_xapasm_fmt(file, name, nxt->global);
            fprintf (file, "\n");
        }
        nxt = nxt->chain;
    }
    fprintf (file, ";LIF .END\n");
    fprintf (file, ";LIF \n");
}

static lif_t *lif_search(const char *name, lif_search_action action)
{
    lif_t *last = NULL;
    lif_t *nxt = head;
    while (nxt)
    {
        if (!strcmp(nxt->name, name))
            return nxt;
        last = nxt;
        nxt = nxt->chain;
    }
    if (action == LIF_SEARCH_ADD)
    {
        tree node = get_identifier(name);
        nxt = xcalloc(1, sizeof(lif_t));
        if (!head) head = nxt;
        else last->chain = nxt;
        nxt->chain = 0;
        nxt->name = ggc_alloc_string(name, -1);
        nxt->node = node;
        if (node && TREE_CODE(node)==IDENTIFIER_NODE)
        {
            if (TREE_CODE_CLASS (TREE_CODE(node)) == 'x')
            {
                node = node_to_ident(node, NULL);
            }
            if (TREE_PUBLIC (node))
                nxt->global = 1;
            if (TREE_PRIVATE (node))
                nxt->local_func = 1;
            switch(TREE_CODE(node))
            {
                case FUNCTION_DECL:
                    nxt->function = 1;
                    break;
                default:
                    break;
            }
        }
        return nxt;
    }
    return NULL;
}

static void debug_lif(FILE *file, lif_t *ptr)
{
    if (ptr && !XAP_FLAG_ENABLED(QUIET) && !quiet_flag)
    {
        fprintf(file, "%s %s function = %d\n",
                asm_comment_start(), ptr->name, ptr->function);
        fprintf(file, "%s %s global = %d\n",
                asm_comment_start(), ptr->name, ptr->global);
        fprintf(file, "%s %s function_addressof = %d\n",
                asm_comment_start(), ptr->name, ptr->function_addressof);
        fprintf(file, "%s %s import = %d\n",
                asm_comment_start(), ptr->name, ptr->import);
        fprintf(file, "%s %s export = %d\n",
                asm_comment_start(), ptr->name, ptr->export);
        fprintf(file, "%s %s referenced = %d\n",
                asm_comment_start(), ptr->name, ptr->referenced);
        fprintf(file, "%s %s local_func = %d\n",
                asm_comment_start(), ptr->name, ptr->local_func);
    }
}

void asm_output_label(FILE *file, const char *name)
{
    lif_t *ptr = lif_search(name, LIF_SEARCH_ADD);
    assemble_name(file, name);
    fputs(":\n", file);
    debug_lif(file, ptr);
    xap_crnt_pattern = NULL;
    xap_penultimate_pattern = NULL;
}

/* called by assemble_name */

static void asm_output_xapasm_fmt(FILE *file, const char *name, int global)
{
    tree id;
    if (*name == '*') abort();
    if (global)
    {
        asm_fprintf (file,  "$%U%s", name);
    }
    else
    {
        const char *ptr = name;
        int valid1 = isalpha((int)*ptr) || (*ptr == '_');
        int valid2 = 0;
        int cnt = 0;
        if (valid1) while (*++ptr)
        {
            if (!isalpha((int)*ptr)) valid2 = 1;
            if (++cnt >= 3) valid2 = 1;
        }
        else
            asm_fprintf (file,  "?");
        if (valid2 || !valid1)
        {
            asm_fprintf (file, "%U%s", name);
        }
        else
        {
            asm_fprintf (file, "%U%s?", name);
        }
    }

    id = maybe_get_identifier (name);
    if (id)
        TREE_SYMBOL_REFERENCED (id) = 1;
}

void asm_output_labelref(FILE *file, const char *name)
{
    {
        lif_t *ptr = lif_search(name, LIF_SEARCH_FIND);
        if (ptr && (*name != '*'))
        {
            ptr->referenced = 1;
            asm_output_xapasm_fmt (file, name, ptr->global);
        }
        else
            asm_output_xapasm_fmt (
                                  file, name, TREE_PUBLIC (get_identifier(name))
                                  );
    }
}

void asm_globalize_label(FILE *file, const char *name)
{
    lif_t *ptr = lif_search(name, LIF_SEARCH_ADD);
    xap2_plus_first_function();
    ptr->export = 1;
    ptr->global = 1;
    if (!XAP_FLAG_ENABLED(QUIET))
    {
        fprintf(file, "%s globalize_label ", asm_comment_start());
        asm_output_labelref(file, name);
        fprintf(file, "\n");
    }
}

void asm_output_external(FILE *file, tree t ATTRIBUTE_UNUSED, const char *name)
{
    lif_t *ptr = lif_search(name, LIF_SEARCH_ADD);
    ptr->import = 1;
    ptr->global = 1;
    if (!XAP_FLAG_ENABLED(QUIET))
    {
        fprintf(file, "%s output_external ", asm_comment_start());
        asm_output_labelref(file, name);
        fprintf(file, "\n");
    }
}

void asm_output_external_libcall(FILE *file ATTRIBUTE_UNUSED ,rtx symref)
{
    lif_t *ptr = lif_search(XSTR(symref,0), LIF_SEARCH_ADD);
    ptr->import = 1;
    ptr->global = 1;
}

void asm_output_local(
        FILE *file, const char *name, int size ATTRIBUTE_UNUSED, int rounded
                     )
{
    xap2_plus_first_function();
    if (!XAP_FLAG_ENABLED(QUIET))
    {
        fprintf(file, "%s output_local ", asm_comment_start());
        assemble_name(file, name);
        fprintf(file, "\n");
    }
    bss_section();
    assemble_name (file, name);		
    if (!XAP_FLAG_ENABLED(XAP2)) fputs(":\n", file);
    fprintf(file, "\tDS\t%d\n", (rounded));
}

void asm_output_pool_prologue(
           const char *name,
           tree decl ATTRIBUTE_UNUSED, int off ATTRIBUTE_UNUSED
                             )
{
    lif_search(name, LIF_SEARCH_ADD);
}

void asm_output_pool_epilogue(
        const char *name,
        tree decl ATTRIBUTE_UNUSED, int off ATTRIBUTE_UNUSED
                             )
{
    lif_search(name, LIF_SEARCH_ADD);
}

void asm_finish_declare_object(
        FILE *file, tree decl,
        int top ATTRIBUTE_UNUSED, int end ATTRIBUTE_UNUSED
                              )
{
    lif_t *ptr = lif_search(IDENTIFIER_POINTER (DECL_NAME (decl)), LIF_SEARCH_ADD);
    xap2_plus_first_function();
    if (DECL_SIZE (decl) && DECL_INITIAL(decl))
    {
        int size_unit, size_bits = 0;
        tree size = DECL_SIZE (decl);
        if (TREE_CODE(size) == INTEGER_CST)
            size_bits = TREE_INT_CST_LOW (size);
        size_unit = size_bits / BITS_PER_UNIT;
        if (size_unit != xap_object_size)
        {
            fprintf(file, "%s object expected %d, actual %d\n",
                    asm_comment_start(), size_unit, xap_object_size);
            if (quiet_flag || XAP_FLAG_ENABLED(QUIET)) abort();
        }
        if (size_unit != xap_expected_size) abort();
        xap_expected_size = 0;
        if (!XAP_FLAG_ENABLED(QUIET))
        {
            fprintf(file, "%s finish_declare_object ", asm_comment_start());
            asm_output_labelref(file, ptr->name);
            fprintf(file, " (siz=%d)\n", size_unit);
        }
    }
}

#ifndef DECL_READONLY_SECTION
#define DECL_READONLY_SECTION(DECL,RELOC) decl_readonly_section(DECL,RELOC)
#endif

static void select_section(
        tree decl, int reloc, unsigned long align ATTRIBUTE_UNUSED
                          )
{
    rtx x;
    int code = TREE_CODE(decl);
    switch (code)
    {
        case STRING_CST:
            xap2_plus_first_function();
            readonly_data_section ();
            break;
        default:
            x = DECL_RTL(decl);
            if (GET_CODE (x) != MEM)
                abort ();
            x = XEXP (x, 0);
            if (GET_CODE (x) != SYMBOL_REF)
                abort ();
            if (first_function) first_global_object_name = XSTR (x, 0);
            /* drop through */
            xap2_plus_first_function();
            if (DECL_READONLY_SECTION (decl, reloc))
                readonly_data_section ();
            else
                data_section ();
            break;
        case CONSTRUCTOR:
        case COMPLEX_CST:
            xap2_plus_first_function();
            readonly_data_section ();
    }
}

void asm_declare_object_name(FILE *file, const char *name, tree decl)
{
    int size_bits = 0;
    tree initial = DECL_INITIAL(decl);
    if (first_function)
        first_global_object_name = name;
    xap2_plus_first_function();
    if (DECL_SIZE (decl) && initial)
    {
        tree size = DECL_SIZE (decl);
        if (TREE_CODE(size) == INTEGER_CST)
            size_bits = TREE_INT_CST_LOW (size);
        assemble_name(file, name);
        if (!XAP_FLAG_ENABLED(XAP2)) fputs(":\n", file);

        if (in_data_section())
        {
            fprintf(file, "\tDS\t%d\n", size_bits / BITS_PER_UNIT);
            initc_section();
        }
    }
    else
    {
        data_section();
        assemble_name(file, name);
        fputs("\t", file);
        fprintf(file, "\tDS\t");
        assemble_name(file, name);
        fputs("_local_end-", file);
        assemble_name(file, name);
        fputs("_local_start\n", file);
        initc_section();
        assemble_name(file, name);
        fputs("_local_start", file);
        fputs("\t", file);
    }
    xap_expected_size = size_bits / BITS_PER_UNIT;
    xap_object_size = 0;
}

void asm_identify_gcc(FILE *file ATTRIBUTE_UNUSED)
{
    if (!quiet_flag) fprintf(stderr, "gcc\n");
}

void *xap2_plus_reg_rtx(int regno)
{
    if (!XAP_FLAG_ENABLED(ENHANCED)) abort();
    switch (regno)
    {
     case RAH: return gen_rtx_MEM(QImode,gen_rtx_SYMBOL_REF(Pmode, "*$XAP_AH"));
     case RAL: return gen_rtx_MEM(QImode,gen_rtx_SYMBOL_REF(Pmode, "*$XAP_AL"));
     case RXH: return gen_rtx_MEM(QImode,gen_rtx_SYMBOL_REF(Pmode, "*$XAP_UXH"));
     case RXL: return gen_rtx_MEM(QImode,gen_rtx_SYMBOL_REF(Pmode, "*$XAP_UXL"));
     case RSP:
     case RFP: return gen_rtx_MEM(QImode,gen_rtx_SYMBOL_REF(Pmode, "*$XAP_UY"));
     case RSPECIAL: return gen_rtx_MEM(QImode, GEN_INT(-9));
     default: abort();
    }
}

rtx maybe_simplify_subreg(
        enum machine_mode m1, rtx subreg,
        enum machine_mode m2, unsigned int byte
                         );
rtx xap_simplify_subreg(rtx subreg);

void xap_file_start(void)
{
    int j;
    unsigned int i;
    max_lines = 1000;
    count = xmalloc(max_lines*sizeof(*count));
    
    for (i = 0; i < max_lines; i++) count[i] = 0;
    
    max_codes = 10;
    count_codes = xmalloc(max_codes*sizeof(*count_codes));
    
    for (j = 0; j < max_codes; j++)
        memset(&count_codes[j], 0, sizeof(*count_codes));

    asm_file_started = 1;
    xap_set_cwd(getpwd());
    first_function = 1;
}

void xap_adjust_type(FILE *file, tree node)
{
    switch (TREE_CODE(node))
    {
        /* It is unsafe to look at any other fields of an ERROR_MARK node.  */
        case ERROR_MARK:
            return;
        case ARRAY_TYPE:
            xap_adjust_type(file, TREE_TYPE(node));
            break;
        case RECORD_TYPE:
            xap_adjust_record(file, node);
            break;
        default:
            break;
    }

}

void xap_adjust_record(FILE *file, tree node)
{
    tree field;
    int first = 1;
    if (!TYPE_NAME(node)) return;

    for (field = TYPE_FIELDS (node); field; field = TREE_CHAIN (field))
    {
        if (!DECL_NAME (field))
            fprintf (file, "; anonymous field");
        else
        {
            if (first)
            {
                first = 0;
                fprintf (file, "%s", IDENTIFIER_POINTER(TYPE_NAME (node)));
                fputs("\tSTRUC\n", file);
            }
            if (TREE_CODE (field) != FIELD_DECL)
            {
                fprintf (file, "; non field ");
                asm_output_xapasm_fmt(file,
                        IDENTIFIER_POINTER (DECL_NAME (field)),
                        TREE_PUBLIC(field));
            }
            else
            {
                tree siz = DECL_SIZE (field);
                asm_fprintf(
                        file, "%U%s", IDENTIFIER_POINTER (DECL_NAME (field))
                           );
                switch(TREE_CODE(TREE_TYPE(field)))
                {
                    case ARRAY_TYPE: 
                        {
                            siz = DECL_SIZE (field);
                            if (!siz)
                            {
                                fprintf (file, "\tDS\t%d\n; ?? array_size", 0);
                                break;
                            }
                        }
                    default:
                        {
                            int bit_offset = 
                               TREE_INT_CST_LOW (DECL_FIELD_BIT_OFFSET (field));
                            int bit_size = TREE_INT_CST_LOW (siz);
                            int word_size = bit_size/BITS_PER_UNIT;

                            if (word_size*BITS_PER_UNIT != bit_size)
                                fprintf (
                                            file,
                                            "\tDS\t%d\n; %d bits",
                                            bit_offset ? word_size
                                                       : word_size+1,
                                            bit_size
                                        );
                            else if (bit_size)
                                fprintf (file, "\tDS\t%d", word_size);
                            else
                                fprintf (file, "; zero size field");
                        }
                }
            }
        }
        fputs("\n", file);
    }
    if (!first)
    {
        fputs("\tENDS\n", file);
    }
    fputs("\n", file);
}

/* don't forget round_up format */
extern tree round_up (tree value, int divisor);

int round_type_align(tree type, int align, int bits ATTRIBUTE_UNUSED)
{
    if (XAP_FLAG_ENABLED(BIG_FUNCTION_PTR) && XAP_FLAG_ENABLED(LARGE))
    {
        if (    (TREE_CODE(type)==POINTER_TYPE)
             && (TREE_CODE(TREE_TYPE(type)) == FUNCTION_TYPE) )
        {
            TYPE_MODE(type) = HImode;
        }
    }
    return align;
}

tree round_type_size(tree type, tree unpadded_size, int align)
{
    if (XAP_FLAG_ENABLED(BIG_FUNCTION_PTR) && XAP_FLAG_ENABLED(LARGE))
    {
        if (
                (TREE_CODE(type)==POINTER_TYPE)
             && (TREE_CODE(TREE_TYPE(type)) == FUNCTION_TYPE)
           )
        {
            TYPE_MODE(type) = HImode;
        }
    }
    return round_up (unpadded_size, align);
}

static void xap_declare_library_function(const char *fptr)
{
    lif_t *ptr = lif_search(fptr, LIF_SEARCH_ADD);
    TREE_PUBLIC(ptr->node) = 1;
    ptr->global = 1;
    ptr->function = 1;
}

void init_target_optabs(void)
{
    set_lang_adjust_rli(xap_adjust_rli);

    TREE_PUBLIC(get_identifier("XAP_AH")) = 1;
    TREE_PUBLIC(get_identifier("XAP_AL")) = 1;
    TREE_PUBLIC(get_identifier("XAP_UXH")) = 1;
    TREE_PUBLIC(get_identifier("XAP_UXL")) = 1;
    TREE_PUBLIC(get_identifier("XAP_UY")) = 1;
    TREE_PUBLIC(get_identifier("saved_y")) = 1;
    xap_declare_library_function("__addsf3");
    xap_declare_library_function("__addsi3");
    xap_declare_library_function("__ashlsi3");
    xap_declare_library_function("__ashrsi3");
    xap_declare_library_function("__cmpsi2");
    xap_declare_library_function("__divhi3");
    xap_declare_library_function("__divsi3");
    xap_declare_library_function("__divsf3");
    xap_declare_library_function("__eqhf2");
    xap_declare_library_function("__eqsf2");
    xap_declare_library_function("__extendhfsf2");
    xap_declare_library_function("__extendhisi2");
    xap_declare_library_function("__fix_truncsfhi2");
    xap_declare_library_function("__fix_truncsfsi2");
    xap_declare_library_function("__fixsfhi");
    xap_declare_library_function("__fixsfsi");
    xap_declare_library_function("__fixuns_truncsfhi2");
    xap_declare_library_function("__fixuns_truncsfsi2");
    xap_declare_library_function("__fixunssfhi");
    xap_declare_library_function("__fixunssfsi");
    xap_declare_library_function("__floathihf");
    xap_declare_library_function("__floathisf");
    xap_declare_library_function("__floatqihf");
    xap_declare_library_function("__floatqisf");
    xap_declare_library_function("__floatsisf");
    xap_declare_library_function("__floatunshihf");
    xap_declare_library_function("__floatunshisf");
    xap_declare_library_function("__floatunsqihf");
    xap_declare_library_function("__floatunsqisf");
    xap_declare_library_function("__floatunssisf");
    xap_declare_library_function("__ftruncsf2");
    xap_declare_library_function("__gehf2");
    xap_declare_library_function("__gesf2");
    xap_declare_library_function("__gthf2");
    xap_declare_library_function("__gtsf2");
    xap_declare_library_function("__lehf2");
    xap_declare_library_function("__lesf2");
    xap_declare_library_function("__lshrsi3");
    xap_declare_library_function("__lthf2");
    xap_declare_library_function("__ltsf2");
    xap_declare_library_function("__modhi3");
    xap_declare_library_function("__modsi3");
    xap_declare_library_function("__mulhi3");
    xap_declare_library_function("__mulsf3");
    xap_declare_library_function("__mulsi3");
    xap_declare_library_function("__negsf2");
    xap_declare_library_function("__negsi2");
    xap_declare_library_function("__nehf2");
    xap_declare_library_function("__nesf2");
    xap_declare_library_function("__subsf3");
    xap_declare_library_function("__truncsfhf2");
    xap_declare_library_function("__udivhi3");
    xap_declare_library_function("__udivsi3");
    xap_declare_library_function("__umodhi3");
    xap_declare_library_function("__umodsi3");
    xap_declare_library_function("puts");
}

static void insn_stats(void)
{
    int j, code;
    for (code = 0; code <= max_code_used; code++)
    {
        int tot = 0;
        switch (count_codes[code].format)
        {
            case INSN_OUTPUT_FORMAT_NONE:
                break;
            case INSN_OUTPUT_FORMAT_SINGLE:
                fprintf(
                        asm_out_file, "%s Single pattern %s ",
                        asm_comment_start(), get_insn_name (code)
                       );
                if (count_codes[code].count_single)
                    fprintf(
                            asm_out_file, "used = %d\n",
                            count_codes[code].count_single
                           );
                else
                    fprintf(asm_out_file, "not used\n");
                break;
            case INSN_OUTPUT_FORMAT_MULTI:
                fprintf(
                        asm_out_file, "%s Multi pattern %s ",
                        asm_comment_start(), get_insn_name (code)
                       );
                for (j = 0; j < max_alternatives; j++)
                    if (count_codes[code].count_multi[j])
                    {
                        tot += count_codes[code].count_multi[j];
                        fprintf(
                                asm_out_file, "used[%d] = %d ",
                                j, count_codes[code].count_multi[j]
                               );
                    }
                if (tot) fprintf(asm_out_file, "total = %d\n", tot);
                else fprintf(asm_out_file, "not used\n");
                break;
            case INSN_OUTPUT_FORMAT_FUNCTION:
                fprintf(
                        asm_out_file, "%s Function pattern %s ",
                        asm_comment_start(), get_insn_name (code)
                       );
                for (j = 0; j < max_alternatives; j++)
                    if (count_codes[code].count_function[j])
                    {
                        tot += count_codes[code].count_function[j];
                        fprintf(
                                asm_out_file, "used[%d] = %d ",
                                j, count_codes[code].count_function[j]
                               );
                    }
                if (tot) fprintf(asm_out_file, "total = %d\n", tot);
                else fprintf(asm_out_file, "not used\n");
                break;
            default:
                abort();
        }
    }
}

void xap_file_end(void)
{
    xap2_plus_first_function();
    if (!XAP_FLAG_ENABLED(QUIET)) xap_dump_rli();
    lif_dump1();
    fprintf(asm_out_file, "\tENDMOD\n");
    lif_dump2();
    insn_stats();
}

void *libcall_value(int m)
{
    enum machine_mode mode = m;
    rtx X;
    switch(mode)
    {
        case HImode: X = gen_rtx(REG, (mode), RAH); break;
        case QImode: X = gen_rtx(REG, (mode), XAP_FLAG_ENABLED(XAP2) ? RAL : RAH); break;
        case HFmode:
                     xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT));
                     X = gen_rtx(REG, (mode), 8);
                     break;
        case SFmode:
                     /* This disagrees with what's returned by
                      * xap_return_in_memory which says that types bigger than 2
                      * words should be returned in memory. perhaps we should
                      * try putting abort() in here */
                     xap2_plus_target_assert(XAP_FLAG_ENABLED(FLOAT));
                     /* FALL THROUGH */
        case SImode:
                     xap2_plus_target_assert(XAP_FLAG_ENABLED(DOUBLE));
                     X = gen_rtx(REG, (mode), 8);
                     break;
        case CSImode:
        case SCmode:
        case DFmode:
        default:
                     abort();
    }
    return (void *)X;
}

int xap2plus_return_in_memory(tree type)
{
    return int_size_in_bytes(type) > 2;
}

rtx function_arg(int cum, enum machine_mode mode, tree decl, int named)
{
    rtx X = 0;
    int siz = ARGUMENT_SIZE(mode,decl);
    switch(siz)
    {
        case 1: 
                    if(cum < (named ? 2 : 1))
                        X = XAP_FLAG_ENABLED(XAP2) 
                            ? gen_rtx_REG(mode, cum ? RAH : RAL)
                            : gen_rtx_REG(mode, cum ? RAL : RAH); 
                    break;
        case 0: X = gen_rtx_REG(mode, RAH); break;
        case 2:
            if (!cum) 
                X = gen_rtx_REG(mode, RAH);
            else if (xap_function_arg_partial_nregs(cum,mode,decl,named))
                X = gen_rtx_REG(QImode,XAP_FLAG_ENABLED(XAP2)?RAH:RAL);
            break;
        case 4:
        case 8:
            break;
        default: if (mode != BLKmode) abort();
    }
    return X;
}

int xap_function_arg_partial_nregs(int cum, enum machine_mode mode, tree decl, int named)
{
    if(   XAP_FLAG_ENABLED(LONG_ARGS_STRADDLE)
       && named
       && cum == 1
       && ARGUMENT_SIZE(mode,decl) == 2) return 1;

    else
        return 0;
}

void xap2_plus_output_source_line (  unsigned int lineno,
        const char *xap_file)
{
    if (lineno != xap_last_linenum)
    {
        unsigned int i;
        if (lineno >= max_lines)
        {
            count = xrealloc(count, lineno*2*sizeof(*count));
            for (i = max_lines; i < lineno*2; i++) count[i] = 0;
            max_lines = lineno*2;
        }
        xap_last_linenum = lineno;

        if (xap_file)
        {
            char *path = xmalloc(strlen(xap_cwd)+strlen(xap_file)+10);
            unsigned long unique_line;
            unique_line = ++count[lineno];
            *path = '\0';
            if(XAP_FLAG_ENABLED(XAP2))
            {
                if (xap_file[0] != '/')
                {
                    strcat(path, xap_cwd);
                    if (path[strlen(path)-1] != '/') strcat(path, "/");
                    strcat(path, xap_file);
                }
                else
                    strcat(path, xap_file);
                
                asm_fprintf(asm_out_file, "^%s.'R:%s'.?%d.%d:\n",
                        module_base_name(), path, lineno, unique_line);
            }
            else
            {
                if (xap_file_changed)
                    /* actually if we get here, the xap2 flag is disabled, which
                     * means it's a c file - but the filename variable is called
                     * xap_file so I named the boolean after it. :-P */
                {
                    char *inhex;
                    char *p;
                    const char *sep =   (
             xap_cwd[0] && (xap_file[0] == '/' || xap_cwd[strlen(xap_cwd)-1] == '/')
                                        )
                                            ? "" : "/";
                    strcat(path, xap_cwd);
                    strcat(path, sep);
                    strcat(path, xap_file);
                    for(p=path;*p;p++) if(*p=='/') *p='\\';
                    inhex = (char*)xmalloc(strlen(path)*2+1);
                    convert_to_hex(path,inhex);
                    asm_fprintf(asm_out_file, "^%s.'%s'.?%d.%d:\n",
                            module_base_name(), inhex, lineno, unique_line);
                    free(inhex);
                    xap_file_changed = false;
                }
                else
                    asm_fprintf(asm_out_file, "^%s.''.?%d.%d:\n",
                            module_base_name(), lineno, unique_line);
            }
            free(path);
        }
    }
}

void xap2_plus_target_assert(int flags)
{
    if (asm_file_started && !flags)
    {
        if (!XAP_FLAG_ENABLED(FLOAT))
            fatal_error(
                    "Attempt to compile floating point code without -float"
                       );
        if (!XAP_FLAG_ENABLED(DOUBLE))
            fatal_error(
                    "Attempt to compile double precision code without -double"
                       );

        fatal_error("Attempt to compile without incorrect target flags");
    }
}

int preferred_debugging_type = DBX_DEBUG;

static int regno_reg_class_array[FIRST_PSEUDO_REGISTER];

void init_once_reg_set()
{
    int i, j;
    static const char initial_fixed_regs[] = \
    { 0, 0, 1, 0, 1, 1, 1, 1, \
        0, 0, 0, 0, 1, 1, 1, 1, \
            0, 0, 0, 0, 0, 0, 1, 1  \
    };
    static const char initial_call_used_regs[] = \
    { 1, 1, 1, 1, 1, 1, 1, 1, \
        1, 1, 1, 1, 1, 1, 1, 1, \
            1, 1, 1, 1, 1, 1, 1, 1  \
    };
#ifdef REG_ALLOC_ORDER
    int myreg_alloc_order[FIRST_PSEUDO_REGISTER] =
    {
        1,  0,  3,
        16, 17, 18, 19, 20, 21, 22, 23,
        8,  9, 10, 11, 12, 13, 14, 15,
        2,  4,  5,  6,  7
    };
    memcpy(reg_alloc_order, myreg_alloc_order, sizeof(reg_alloc_order));
#endif
    memcpy (fixed_regs, initial_fixed_regs, sizeof fixed_regs);
    memcpy (call_used_regs, initial_call_used_regs, sizeof call_used_regs);

    SET_HARD_REG_BIT (reg_class_contents[ADDR_REGS], RXL);
    SET_HARD_REG_BIT (reg_class_contents[DATA_REGS], RAL);
    SET_HARD_REG_BIT (reg_class_contents[DATA_REGS], RAH);

    for (i = 0; i < 7; i++)
        SET_HARD_REG_BIT (reg_class_contents[CHIP_REGS], i);
    for (i = 8; i < 16; i++)
        SET_HARD_REG_BIT (reg_class_contents[FAKE_REGS], i);
    for (i = 16; i < 24; i++)
        SET_HARD_REG_BIT (reg_class_contents[FRAME_REGS], i);
    for (i = 0; i < FIRST_PSEUDO_REGISTER; i++)
        SET_HARD_REG_BIT (reg_class_contents[ALL_REGS], i);
    /* initialize regno_reg_class */
    for (i = 0; i < N_REG_CLASSES; i++)
        for (j = 0; j < FIRST_PSEUDO_REGISTER; j++)
            if (TEST_HARD_REG_BIT (reg_class_contents[i], j))
            {
                if (regno_reg_class_array[j]==NO_REGS)
                    regno_reg_class_array[j] = i;
            }
}

int regno_reg_class(int regno)
{
    return (((unsigned)regno) < FIRST_PSEUDO_REGISTER)
                ? regno_reg_class_array[regno] : NO_REGS;
}

char *asm_format_private_name(
            void *output, const char * const name, int labelno
                             )
{
    sprintf(output, "%s?%d?", name, labelno);
    return (char *)output;
}

void override_options(switch_t p)
{
    extern int flag_leading_underscore;
    /* Gross! Steal a pointer to a static funciton in toplev.c */
    print_switch_values_ptr = p;
    if (XAP_FLAG_ENABLED(ENHANCED))
    {
        SET_HARD_REG_BIT (reg_class_contents[FAKE_REGS], RXL);
        SET_HARD_REG_BIT (reg_class_contents[FAKE_REGS], RAL);
        SET_HARD_REG_BIT (reg_class_contents[FAKE_REGS], RAH);
    }
    if (!quiet_flag || !XAP_FLAG_ENABLED(QUIET))
    {
        flag_print_asm_name = 1;
        flag_verbose_asm = 1;
        if (!quiet_flag && !XAP_FLAG_ENABLED(QUIET)) flag_dump_rtl_in_asm = 1;
    }
    flag_leading_underscore = XAP_FLAG_ENABLED(UNDERSCORE) || !XAP_FLAG_ENABLED(XAP2);
    if (debug_info_level == DINFO_LEVEL_TERSE)
        debug_info_level = DINFO_LEVEL_NORMAL;
    if (flag_writable_strings)
    {
        fprintf(stderr, "Unsupported option -fwritable_strings turned off\n");
        flag_writable_strings = 0;
    }
    if (!XAP_FLAG_ENABLED(QUIET))
    {
        if (!XAP_FLAG_ENABLED(XAP2))
        {
            fprintf(
               stderr, "-xap2 not specified. No struct symbols will be output\n"
                   );
        }
        if (!XAP_FLAG_ENABLED(FLOAT))
        {
            fprintf(
                  stderr, "-float not specified. floating point not supported\n"
                   );
        }
        if (!XAP_FLAG_ENABLED(DOUBLE))
        {
            fprintf(
               stderr, "-double not specified. double precision not supported\n"
                   );
        }
    }
    if (XAP_FLAG_ENABLED(BIG_FUNCTION_PTR) && !XAP_FLAG_ENABLED(LARGE))
    {
        fprintf(stderr,"Can't use -mbig-function-pointers without -mlarge-mode\n");
        abort();
    }
    if (write_symbols == DBX_DEBUG && !XAP_FLAG_ENABLED(STABS) && XAP_FLAG_ENABLED(XAP2)) 
    {
        write_symbols = NO_STABS;
    }

    memset(real_format_for_mode, 0, sizeof real_format_for_mode);
#ifdef REAL_MODE_FORMAT
    REAL_MODE_FORMAT(HFmode) = &ieee_single_format;
    REAL_MODE_FORMAT(SFmode) = &ieee_double_format;
#else
    real_format_for_mode[HFmode-QFmode] = &ieee_single_format;
    real_format_for_mode[SFmode-QFmode] = &ieee_double_format;
#endif

    /* Always enable peephole2 RTL-to-RTL transformations */
    flag_peephole2 = 1;
}

/* Compute the cost of computing a constant rtl expression RTX
   whose rtx-code is CODE.  The body of this macro is a portion
   of a switch statement.  If the code is computed here,
   return it with a return statement.  Otherwise, break from the switch. 

   -1, 0, 1 are cheaper for add, sub ... 
   */

int register_move_cost(
        enum machine_mode MODE, enum reg_class class1, enum reg_class class2
                      )
{
    if (reload_in_progress && !quiet_flag)
    {
        fprintf(
                stderr,
                "register_move_cost(enum machine_mode %d, enum reg_class %s, enum reg_class %s)\n",
                MODE, reg_class_names[class1], reg_class_names[class2]
               );
    }
    switch (class1)
    {
        case ADDR_REGS:
        case DATA_REGS:
        case CHIP_REGS:
            switch (class2)
            {
                case ADDR_REGS:
                case DATA_REGS:
                case CHIP_REGS:
                    return XAP_FLAG_ENABLED(ENHANCED) ? COSTS_N_INSNS(1)
                                           : COSTS_N_INSNS(2);
                case FAKE_REGS:
                case FRAME_REGS:
                case NO_REGS:
                    return COSTS_N_INSNS(1);
                case ALL_REGS:
                    return COSTS_N_INSNS(3);
                default: abort();
            }
        case NO_REGS:
        case FAKE_REGS:
        case FRAME_REGS:
            switch (class2)
            {
                case ADDR_REGS:
                case DATA_REGS:
                case CHIP_REGS:
                    return COSTS_N_INSNS(1);
                case FAKE_REGS:
                case FRAME_REGS:
                case NO_REGS:
                    return COSTS_N_INSNS(3);
                case ALL_REGS:
                    return COSTS_N_INSNS(3);
                default: abort();
            }
        case ALL_REGS:
            return COSTS_N_INSNS(3);
        default: abort();
    }
    abort();
}

int memory_move_cost(
            enum machine_mode MODE, enum reg_class class, int DIRECTION
                    )
{
    if (reload_in_progress && !quiet_flag)
    {
        fprintf(
    stderr,
    "memory_move_cost(enum machine_mode %d, enum reg_class %s, direction %d)\n",
    MODE,reg_class_names[class], DIRECTION
               );
    }
    return COSTS_N_INSNS(1);
}

int default_rtx_costs(rtx X, int outer_code)
{
    int total = -1;
    int code = GET_CODE(X);
    switch(code)
    {
        case PLUS:
            total = 10;
            break;
        case CALL:
            if (CONSTANT_P (XEXP (XEXP (X, 0), 0)))
                total = 0;
            else
                total = 10;
            break;
        case CONST_INT:						
            if(-128 <= INTVAL(X) && INTVAL(X) <= 127)               
                total =  0;                                                 
            else if(-32768 <= INTVAL(X) && INTVAL(X) <= 32767)           
                total =  1;                                                 
            else total =  2;
            break;
        case CONST:
            total = default_rtx_costs(XEXP(X, 0), outer_code);
        case LABEL_REF:						
        case SYMBOL_REF:						
            total =  2;
            break;
        case CONST_DOUBLE:						
            total =  8;
            break;
        case MULT:								
            total = COSTS_N_INSNS(8);						
            break;								
        case DIV:								
            total = COSTS_N_INSNS (12);					        
            break;								
        case MOD:								
            total = COSTS_N_INSNS (12);					        
            break;								
        case ABS:								
            total = COSTS_N_INSNS (4);						
            break;								
        case ZERO_EXTEND:							
            /* only used for: qi->hi */						
            total = COSTS_N_INSNS(2);						
            break;								
        case SIGN_EXTEND:							
            if (GET_MODE(X) == HImode)						
                total = COSTS_N_INSNS(4);					
            else								
                total = COSTS_N_INSNS(8);					
            break;								
        case ASHIFT:								
        case LSHIFTRT:							
        case ASHIFTRT:							
            if (GET_MODE(X) ==  QImode)					        
                total = COSTS_N_INSNS(4); /* worst case */ 			
            else if (GET_MODE(X) == HImode)					
                total = COSTS_N_INSNS(6);					        
            else								
                total = COSTS_N_INSNS(10);
            break;
    }
    return total;
}

extern FILE *rtl_dump_file;

static void output_function_prologue(void *f, long size
#ifdef CONST_VECTOR_NUNITS
#else
        , void *first, int last_linenum, char *last_filename
#endif
        )
{
    FILE *stream = (FILE *)f;
    int off = size+current_function_outgoing_args_size+function_overhead;
#ifdef CONST_VECTOR_NUNITS
#else
    xap2_plus_output_source_line(last_linenum, last_filename);
#endif

    /* current_function_pretend_args_size is "# bytes the prologue should push
     * and pretend that the caller pushed them." Who am I to argue?
     */
    if(current_function_pretend_args_size)
    {
        asm_fprintf(stream,
                    "\tsub\tY,#%d\n",
                    current_function_pretend_args_size);
    }
    
    if (XAP_FLAG_ENABLED(FRAME_OPTIM)) asm_fprintf(stream, "\tprologue\t%d\n", off);

    else if(   XAP_FLAG_ENABLED(LEAF_OPTIM)
           && current_function_is_leaf
           && ((XAP_FLAG_ENABLED(LARGE) && off==2) || off==1)
           && (!XAP_FLAG_ENABLED(LARGE) || !regs_ever_live[RXH])
           && !regs_ever_live[RXL] )
    {
        /* TODO: we could be a bit more fine grained in non-enhanced mode. */
        asm_fprintf(stream,  "; leaf function with empty stack frame\n");
    }
    else if (XAP_FLAG_ENABLED(ENHANCED))
    {
        if (XAP_FLAG_ENABLED(LARGE))
            asm_fprintf(stream, "\tenterl\t%s%d\n", (XAP_FLAG_ENABLED(XAP2))?"":"#",off);
        else
            asm_fprintf(stream, "\tenter\t%s%d\n", (XAP_FLAG_ENABLED(XAP2))?"":"#", off);
    }
    else
    {
        if (XAP_FLAG_ENABLED(LARGE)) asm_fprintf(stream, "\tst\txh,@(-1,Y)\n");
        asm_fprintf(stream, "\tst\tx,@(-2,Y)\n");
        asm_fprintf(stream, "\tsub\ty,#%d\n", off);
    }
    fprintf(stream, "?LBB_%s:\n", IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(cfun->decl)));
    cc_status_mdep_all_init(2);
    xap_crnt_pattern = NULL;
    xap_penultimate_pattern = NULL;
}

static void output_function_epilogue(void *f, long sz
#ifdef CONST_VECTOR_NUNITS
#else
        , int last_linenum, char *last_filename
#endif
        )
{
    FILE *stream = (FILE *)f;
    int off = sz+current_function_outgoing_args_size+function_overhead;
    fprintf(stream, "?LBE_%s:\n",IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(cfun->decl)));
    
    if (XAP_FLAG_ENABLED(FRAME_OPTIM))
        asm_fprintf(stream, "\tepilogue\t%d\n", off);
    else if(   XAP_FLAG_ENABLED(LEAF_OPTIM)
           && current_function_is_leaf
           && ((XAP_FLAG_ENABLED(LARGE) && off==2) || off==1)
           && (!XAP_FLAG_ENABLED(LARGE) || !regs_ever_live[RXH])
           && !regs_ever_live[RXL] )
    {
        /* TODO: we could be a bit more fine grained in non-enhanced mode. */
        if(current_function_pretend_args_size)
            asm_fprintf(stream,"\tadd\ty,#%d\n",current_function_pretend_args_size);
        asm_fprintf(stream,"\tbra\t0,X\n");
    }
    else if (XAP_FLAG_ENABLED(ENHANCED) && !current_function_pretend_args_size)
    {
        if (XAP_FLAG_ENABLED(LARGE))
                asm_fprintf(stream, "\tleavel\t%s%d\n", (XAP_FLAG_ENABLED(XAP2))?"":"#", off);
        else
                asm_fprintf(stream, "\tleave\t%s%d\n", (XAP_FLAG_ENABLED(XAP2))?"":"#", off);
    }
    else
    {
        /* current_function_pretend_args_size is "# bytes the prologue should push
         * and pretend that the caller pushed them." We pushed these bytes
         * before doing storing X/XH, so we need to be careful about where we
         * find X/XH now. */
        asm_fprintf(stream, "\tadd\ty,#%d\n", off + current_function_pretend_args_size);
        if (XAP_FLAG_ENABLED(LARGE))
            asm_fprintf(stream, "\tld\txh,@(%d,Y)\n", (-1) - current_function_pretend_args_size);
        asm_fprintf(stream, "\tbra\t@(%d,Y)\n", (-2) - current_function_pretend_args_size);
    }
    cc_status_mdep_all_init(3);
#ifdef CONST_VECTOR_NUNITS
#else
    xap2_plus_output_source_line(last_linenum, last_filename);
#endif
}

int hard_register_operand(rtx op, enum machine_mode mode)
{
    return    register_operand(op, mode)
           && (    (REGNO(op) < 8) 
                || (REGNO(op)==10000)
                || !no_new_pseudos 
                || reload_in_progress );
}

int reload_operand(rtx op ATTRIBUTE_UNUSED, enum machine_mode mode ATTRIBUTE_UNUSED)
{ return reload_in_progress; }

int reload_in_operand(rtx op ATTRIBUTE_UNUSED, enum machine_mode mode ATTRIBUTE_UNUSED)
{ return reload_in_progress; }

int reload_out_operand(rtx op ATTRIBUTE_UNUSED, enum machine_mode mode ATTRIBUTE_UNUSED)
{ return reload_in_progress; }

int class_likely_spilled(enum reg_class class)
{
    switch (class)
    {
        case ADDR_REGS: return 1;
        case DATA_REGS: return 1;
        case CHIP_REGS: return 1;
        case FAKE_REGS: return 0;
        case FRAME_REGS: return 0;
        case ALL_REGS: return 0;
        default: abort();
    }
}

void final_prescan_insn(rtx insn, 
        rtx *operands ATTRIBUTE_UNUSED,
        int n ATTRIBUTE_UNUSED,
        int last_linenum, const char *last_filename)
{
    int code = recog_memoized(insn);
    if (xap_crnt_pattern != PATTERN(insn))
        {
        xap_penultimate_pattern = xap_crnt_pattern;
        xap_crnt_pattern = PATTERN(insn);
        }
    if (code >= max_codes)
    {
        int i;
        count_codes = xrealloc(count_codes, code*2*sizeof(*count_codes));
        for (i = max_codes; i < code*2; i++)
            memset(&count_codes[i], 0, sizeof(*count_codes));
        max_codes = code*2;
    }
    if (code > max_code_used)
        max_code_used = code;
    if (which_alternative >= max_alternatives) abort();
    count_codes[code].format = insn_data[code].output_format;
    switch (count_codes[code].format)
    {
        case INSN_OUTPUT_FORMAT_NONE:
            abort();
            break;
        case INSN_OUTPUT_FORMAT_SINGLE:
            ++count_codes[code].count_single;
            break;
        case INSN_OUTPUT_FORMAT_MULTI:
            ++count_codes[code].count_multi[which_alternative];
            break;
        case INSN_OUTPUT_FORMAT_FUNCTION:
            ++count_codes[code].count_function[which_alternative];
            break;
        default:
            abort();
    }
    xap2_plus_output_source_line(last_linenum, last_filename);
}

static bool xap_assemble_integer (
        rtx x ,
        unsigned int size ,
        int aligned_p )
{
    REAL_VALUE_TYPE u;
    xap_assembled_integer = 1;

    if(size<xap_thissize)
    {
        /* This should only happen when assembling things in two chunks.
         * assemble_real in varasm.c does this. */
        xap_thissize -= size;
        xap_excess_size = 0;
    }
    else xap_excess_size = size - xap_thissize;

    if (!XAP_FLAG_ENABLED(QUIET))
    {
        fprintf(asm_out_file, "%s assemble integer (%d,%d)\n", 
                asm_comment_start(), size, xap_thissize);
        fprintf(stderr, "xap_assembled_integer size %d\n", size);
        debug_rtx(x);
    }
    switch (GET_CODE(x))
    {
        case SYMBOL_REF:
            switch (size)
            {
                case 1:
                    {
                        lif_t *ptr = lif_search(XSTR(x,0), LIF_SEARCH_ADD);
                        if (ptr && ptr->function && XAP_FLAG_ENABLED(LARGE))
                        {
                            xap_object_size += 1;
                            fprintf(asm_out_file, "\tdc\tlwrd(");
                            asm_output_symbolref(asm_out_file, XSTR(x,0));
                            fprintf(asm_out_file, "\?\?)\n");
                            ptr->function_addressof = 1;
                        }
                        else
                        {
                            xap_object_size += 1;
                            fprintf(asm_out_file, "\tdc\t");
                            assemble_name(asm_out_file, XSTR(x,0));
                            fprintf(asm_out_file, "\n");
                        }
                        debug_lif(asm_out_file, lif_search(XSTR(x,0), LIF_SEARCH_FIND));
                        break;
                    }
                case 2:
                    xap_object_size += 2;
                    fputs ("\tdc\thwrd(", asm_out_file);
                    assemble_name(asm_out_file, XSTR(x, 0));
                    fputs (")\n\tdc\tlwrd(", asm_out_file);
                    assemble_name(asm_out_file, XSTR(x, 0));
                    fputs (")\n", asm_out_file);
                    break;
                case 4:
                    break;
                case 8:
                    break;
            }
            break;
        case CONST_INT:
            switch (size)
            {
                case 1:
                    xap_object_size += 1;
                    fprintf(asm_out_file,  "\tdc\tH'%.4X\n", INTVAL(x)&0xFFFF);
                    break;
                case 2:
                    xap_object_size += 2;
                    fprintf(asm_out_file,  "\tdc\tH'%.4X\n\tdc\tH'%.4X\n", 
                            (INTVAL(x)>>16)&0xFFFF, INTVAL(x)&0xFFFF);
                    break;
                case 4:
                    xap_object_size += 4;
                    if(INTVAL(x)<0)
                        fprintf(asm_out_file,  "\tdc\tH'FFFF\n\tdc\tH'FFFF\n");
                    else
                        fprintf(asm_out_file,  "\tdc\tH'0000\n\tdc\tH'0000\n");
                    fprintf(asm_out_file,  "\tdc\tH'%.4X\n\tdc\tH'%.4X\n\n", 
                            (INTVAL(x)>>16)&0xFFFF, INTVAL(x)&0xFFFF);
                    break;
                case 8:
                    break;
            }
            break;
        case CONST:
            return xap_assemble_integer(XEXP(x, 0), size, aligned_p);
            break;
        case SUBREG:
            {
                rtx sym; int val;
                /* We only know how to deal with things like:
                 * (SUBREG (PLUS (SYMBOL_REF ..) (CONST_INT ..)) ..)
                 * Which we get when someone does something like bitpack
                 * function pointers with constant values. We abort if it
                 * doesn't look like that.
                 */
                if(   GET_CODE(XEXP(x,0)) != PLUS
                   || GET_CODE(XEXP(XEXP(x,0),0)) !=  SYMBOL_REF
                   || GET_CODE(XEXP(XEXP(x,0),1)) !=  CONST_INT) abort();

                sym = XEXP(XEXP(x,0),0);
                val = INTVAL(XEXP(XEXP(x,0),1));

                if(XINT(x,1)==1) /* lwrd */
                {
                    fprintf(asm_out_file,"\tdc\tlwrd(");
                    assemble_name(asm_out_file, XSTR(sym, 0));
                    fprintf(asm_out_file,"+ H'%.8X)\n",val);
                    xap_object_size += 1;
                }
                else if(XINT(x,1)==0) /* hwrd */
                {
                    fprintf(asm_out_file,"\tdc\thwrd(");
                    assemble_name(asm_out_file, XSTR(sym, 0));
                    fprintf(asm_out_file,"+ H'%.8X)\n",val);
                    xap_object_size += 1;
                }
                else abort(); /* Shouldn't get here - something broke if we do */
                break;
            }
        case PLUS: 
            {
                rtx lnk1 = XEXP(x,0);
                if (GET_MODE(x) == HImode)
                {
                    if(size!=2) abort(); /* something bad happened */
                    xap_assemble_integer(simplify_gen_subreg(QImode,x,HImode,0),1,aligned_p);
                    xap_assemble_integer(simplify_gen_subreg(QImode,x,HImode,1),1,aligned_p);
                    break;
                }
                else if (GET_CODE(lnk1)==ZERO_EXTEND)
                {
                    rtx lnk2 = XEXP(lnk1,0);
                    if (GET_CODE(lnk2)==SUBREG)
                    {
                        rtx lnk3 = XEXP(lnk2,0);
                        if (GET_CODE(lnk3)==SYMBOL_REF)
                        {
                            rtx lnk4 = XEXP(x,1);
                            xap_object_size += 2;
                            fputs ("\tdc\thwrd(", asm_out_file);
                            assemble_name(asm_out_file, XSTR(lnk3, 0));
                            switch (GET_CODE(lnk4))
                            {
                                case CONST_INT:
                                    fprintf(
                                            asm_out_file,
                                            ") + H'%.4X\n", 
                                            (INTVAL(lnk4)>>16)&0xFFFF
                                           );
                                    break;
                                default: abort();
                            }
                            fputs ("\tdc\tlwrd(", asm_out_file);
                            assemble_name(asm_out_file, XSTR(lnk3, 0));
                            switch (GET_CODE(lnk4))
                            {
                                case CONST_INT:
                                    fprintf(
                                            asm_out_file,
                                            ") + H'%.4X\n",
                                            INTVAL(lnk4)&0xFFFF
                                           );
                                    break;
                                default: abort();
                            }
                            break;
                        }
                    }
                }
                xap_object_size += 1;
                fputs ("\tdc\t", asm_out_file);
                print_memory(asm_out_file, XEXP(x, 0), 0, ' ');
                fputs (" + (", asm_out_file);
                print_memory(asm_out_file, XEXP(x, 1), 0, ' ');
                fputs (")\n", asm_out_file);
                break;
            }
        case CONST_DOUBLE:
            if (GET_MODE(x)==VOIDmode) 
            {
                /* ummm.. dunno what it means if size isn't 4 */
                if(size!=4) abort();
                xap_object_size += 4;
                /* special case for CONST_DOUBLE, actually represents integer */
                fprintf (
                     asm_out_file, 
                     "\tdc\tH'%.4X\n\tdc\tH'%.4X\n\tdc\tH'%.4X\n\tdc\tH'%.4X\n",
                     (CONST_DOUBLE_HIGH (x) >> 16)&0xFFFF,
                     CONST_DOUBLE_HIGH (x)&0xFFFF,
                     (CONST_DOUBLE_LOW (x) >> 16)&0xFFFF,
                     CONST_DOUBLE_LOW (x)&0xFFFF
                        );
            }
            else
            { 
                /* Really is floating point */
                memcpy ((char *) &u,
                        (char *) &CONST_DOUBLE_LOW (x),
                        sizeof u);
                switch (size)
                {
                    case 1:
                        /* TODO: abort(); ? */
                        fprintf(
                           asm_out_file,
                           "\tdc\t0 ;!! apparently outputting float of size 1\n"
                               );
                        debug_rtx(x);
                        break;
                    case 2: output_short_float_xap(asm_out_file, u); break;
                    case 4: output_float_xap(asm_out_file, u); break;
                    case 8: break;
                }
                break;
                default:
                abort();
                break;
            }
    }
    return true;
}

int asm_output_special_pool_entry(
            FILE *file, rtx x, int mode, int align, int labelno
                                 )
{
    assemble_align (align);
    /* Output the label.  */
    asm_output_internal_label (file, "LC", labelno);
    switch (GET_MODE_CLASS(mode))
    {
        case MODE_FLOAT:
        case MODE_INT:
        case MODE_PARTIAL_INT:
            xap_assemble_integer(x, GET_MODE_SIZE(mode), align);
            break;
#ifdef CONST_VECTOR_NUNITS
        case MODE_VECTOR_FLOAT:
        case MODE_VECTOR_INT:
            {
                int i, units;
                rtx elt;

                units = CONST_VECTOR_NUNITS (x);

                for (i = 0; i < units; i++)
                {
                   elt = CONST_VECTOR_ELT (x, i);
                   xap_assemble_integer (elt, GET_MODE_UNIT_SIZE (mode), align);
                }
            }
            break;
#endif
        default:
            return 0;
    }
    return 1;
}

void asm_output_align(FILE *file ATTRIBUTE_UNUSED, int log ATTRIBUTE_UNUSED)
{
    xap2_plus_first_function();
}

static void asm_output_space(
            FILE *file, const char *name, int size ATTRIBUTE_UNUSED, int rounded
                            )
{
    int file_scope;
    tree decl = get_identifier(name);
    tree global = node_to_ident (decl, &file_scope);
    tree typ = TREE_TYPE(global);
    if (XAP_FLAG_ENABLED(XAP2) && (TREE_CODE(typ)==RECORD_TYPE) && file_scope)
        xap_dump_rli();

    assemble_ident (file, get_identifier(name));
    if (!XAP_FLAG_ENABLED(XAP2)) fputs(":\n", file);
    fprintf(file, "\tDS\t%d\n", rounded);
}

void asm_output_common(FILE *file, const char *name, int size, int rounded)
{
    lif_t *ptr = lif_search(name, LIF_SEARCH_ADD);
    ptr->export = 1;
    xap2_plus_first_function();
    bss_section();
    asm_output_space (file, name, size, rounded);		
}

static unsigned int xap_elf_section_type_flags(
                    tree decl, const char *name, int reloc
                                              )
{
    unsigned int flags = default_section_type_flags (decl, name, reloc);
    return flags;
}

static void xap_encode_section_info(
            tree decl, int first_declaration ATTRIBUTE_UNUSED
                                   )
{
    const char *name;
    lif_t *ptr;
    if (asm_out_file) switch (TREE_CODE(decl))
    {
        case FUNCTION_DECL:
            name = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (decl));
            ptr = lif_search(name, LIF_SEARCH_ADD);
            if (!XAP_FLAG_ENABLED(QUIET))
                fprintf(
                        asm_out_file,
                        "%s Function %s\n", asm_comment_start(), name
                       );
            
            break;
        case VAR_DECL:
            name = IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (decl));
            ptr = lif_search(name, LIF_SEARCH_ADD);
            if (!XAP_FLAG_ENABLED(QUIET))
            {
                fprintf(asm_out_file,"%s Variable ", asm_comment_start());
                asm_output_xapasm_fmt(asm_out_file, name, TREE_PUBLIC (decl));
                fprintf(asm_out_file,"\n");
            }
            break;
        default:
            break;
    }
}

#ifdef CONST_VECTOR_NUNITS
#include "target.h"
#include "target-def.h"

/* Initialize the GCC target structure.  */

#undef  TARGET_ASM_OPEN_PAREN
#define TARGET_ASM_OPEN_PAREN "("
#undef  TARGET_ASM_CLOSE_PAREN
#define TARGET_ASM_CLOSE_PAREN ")"

#undef TARGET_ASM_INTEGER
#undef TARGET_ASM_FUNCTION_PROLOGUE
#undef TARGET_ASM_FUNCTION_EPILOGUE
#undef TARGET_ASM_FUNCTION_END_PROLOGUE
#undef TARGET_ASM_FUNCTION_BEGIN_EPILOGUE
#undef TARGET_SECTION_TYPE_FLAGS
#undef TARGET_ASM_GLOBALIZE_LABEL
#undef TARGET_ENCODE_SECTION_INFO
#undef TARGET_ASM_SELECT_SECTION
#undef TARGET_COMP_TYPE_ATTRIBUTES
#undef TARGET_SET_DEFAULT_TYPE_ATTRIBUTES
#undef TARGET_ASM_FILE_START
#undef TARGET_ASM_FILE_END

#ifdef GCC_3P4
static int xap2plus_vector_opaque(tree _1 ATTRIBUTE_UNUSED)
{ return 0; }

static void xap2plus_machine_dependent_reorg(void)
{ machine_dep_reorg(get_insns()); }

static void xap2plus_internal_label(FILE *_1, const char *_2, unsigned long _3)
{ asm_output_internal_label(_1, _2, _3); }

static bool xap2plus_rtx_costs(rtx _1, int _2 ATTRIBUTE_UNUSED, int _3, int *_4)
{
    int total = default_rtx_costs(_1, _3);
    if (total == -1) return false;
    *_4 = total;
    return true;
}

static int xap2plus_address_cost(rtx _1)
{ return rtx_cost (_1, MEM); }

static void xap2plus_init_libfuncs(void)
{ init_target_optabs(); }

#undef TARGET_VECTOR_OPAQUE_P
#undef TARGET_MACHINE_DEPENDENT_REORG
#undef TARGET_ASM_INTERNAL_LABEL
#undef TARGET_ADDRESS_COST
#undef TARGET_RTX_COSTS
#undef TARGET_INIT_LIBFUNCS

#define TARGET_VECTOR_OPAQUE_P xap2plus_vector_opaque
#define TARGET_MACHINE_DEPENDENT_REORG xap2plus_machine_dependent_reorg
#define TARGET_ASM_INTERNAL_LABEL xap2plus_internal_label
#define TARGET_ADDRESS_COST xap2plus_address_cost
#define TARGET_RTX_COSTS xap2plus_rtx_costs
#define TARGET_INIT_LIBFUNCS xap2plus_init_libfuncs
#endif /* ifdef GCC_3P4 */

static void xap2plus_function_prologue PARAMS ((FILE *_1, HOST_WIDE_INT _2))
{
    output_function_prologue(_1,_2);
}

static void xap2plus_function_epilogue PARAMS ((FILE *_1, HOST_WIDE_INT _2))
{
    output_function_epilogue(_1,_2);
}

static bool xap2plus_assemble_integer PARAMS ((rtx _1, unsigned int _2, int _3))
{
    return xap_assemble_integer(_1,_2,_3);
}

static void xap2plus_asm_to_stream PARAMS ((FILE *_1 ATTRIBUTE_UNUSED))
{
    xap2_plus_output_source_line(xap_last_linenum, xap_file);
}

static unsigned int xap2plus_section_type_flags PARAMS ((union tree_node *_1, const char *_2, int _3))
{
    return xap_elf_section_type_flags(_1,_2,_3);
}

static void xap2plus_globalize_label PARAMS ((FILE *_1, const char *_2))
{
    asm_globalize_label(_1,_2);
}

static void xap2plus_encode_section_info PARAMS ((tree _1, int _2))
{
    xap_encode_section_info(_1,_2);
}

static void xap2plus_select_section(tree _1, int _2, unsigned HOST_WIDE_INT _3)
{
    select_section(_1, _2, _3);
}

static int xap2plus_comp_type_attributes(tree _1 ATTRIBUTE_UNUSED, tree _2 ATTRIBUTE_UNUSED)
{
    return 1;
}

static void xap2plus_set_default_type_attributes(tree t ATTRIBUTE_UNUSED)
{
}

#define TARGET_ASM_INTEGER xap2plus_assemble_integer
#define TARGET_ASM_FUNCTION_PROLOGUE xap2plus_function_prologue
#define TARGET_ASM_FUNCTION_EPILOGUE xap2plus_function_epilogue
#define TARGET_ASM_FUNCTION_END_PROLOGUE xap2plus_asm_to_stream
#define TARGET_ASM_FUNCTION_BEGIN_EPILOGUE xap2plus_asm_to_stream
#define TARGET_SECTION_TYPE_FLAGS xap2plus_section_type_flags
#define TARGET_ASM_GLOBALIZE_LABEL xap2plus_globalize_label
#define TARGET_ENCODE_SECTION_INFO xap2plus_encode_section_info
#define TARGET_ASM_SELECT_SECTION xap2plus_select_section
#define TARGET_COMP_TYPE_ATTRIBUTES xap2plus_comp_type_attributes
#define TARGET_SET_DEFAULT_TYPE_ATTRIBUTES xap2plus_set_default_type_attributes
#define TARGET_ASM_FILE_START xap_file_start
#define TARGET_ASM_FILE_END xap_file_end

struct gcc_target targetm = TARGET_INITIALIZER;
#endif

const char *xap2_plus_call (rtx *operands)
{
    operands[8] = gen_rtx(REG, QImode, FIRST_PSEUDO_REGISTER-2);
    operands[9] = gen_rtx(REG, QImode, FIRST_PSEUDO_REGISTER-1);
    operands[3] = gen_rtx(REG, QImode, RXH);
    operands[4] = gen_rtx(REG, QImode, RXL);
    switch(GET_CODE(operands[0]))
    {
        case SYMBOL_REF: return "bsr\t%c0";
        case MEM:
        case PLUS:
            if (XAP_FLAG_ENABLED(BIG_FUNCTION_PTR))
            {
                if(GET_MODE(operands[0])!=HImode) abort();

                operands[5] = XEXP(operands[0],0);
                operands[6] = XEXP(operands[5],0);
                operands[7] = XEXP(operands[5],1);

                if (   (   (GET_CODE(operands[5])==PLUS) 
                        && rtx_equal_p(operands[6],frame_pointer_rtx)
                        && (GET_CODE(operands[7])==CONST_INT))
                    || rtx_equal_p(operands[5],frame_pointer_rtx))
                {
                    return "ld\t%3,%t0\n\tbsr\t%b0";
                }
                else if (GET_CODE(operands[5])==PLUS)
                {
                    operands[6] =
                        gen_rtx_MEM(HImode, gen_rtx(REG, QImode, RXL));
                    return
                        "ld\t%4,%6\n\tadd\t%4,%6\n\tadd\t%4,%7\n\tst\t%4,%9\n"
                        "\tld\t%4,%t6\n\tst\t%4,%8\n\tld\t%4,%9\n\tld\t%4,%b6\n"
                        "\tst\t%4,%9\n\tld\t%3,%8\n\tbsr\t%9";
                }
                else
                    return "ld\t%4,%b0\n\tst\t%4,%9\n\tld\t%4,%t0\n"
                           "\tst\t%4,%8\n\tld\t%3,%8\n\tbsr\t%9";
                    
            }
            else
            {
                if(GET_MODE(operands[0])!=QImode) abort();
                else
                {
                    operands[5] = XEXP(operands[0],0);
                    operands[6] = XEXP(operands[5],0);
                    operands[7] = XEXP(operands[5],1);

                    if (    (GET_CODE(operands[5])==PLUS) 
                         && rtx_equal_p(operands[6],frame_pointer_rtx)
                         && (GET_CODE(operands[7])==CONST_INT))
                        return "bsr\t%0";
                    
                    else if (   GET_CODE(operands[5])==PLUS
                             || GET_CODE(operands[0])==MEM  )
                        return "ld\t%4,%0\n\tbsr\t0,%4";

                    else
                        return "bsr\t%0";
                }
            }
                         break;
        case REG:
                         if (XAP_FLAG_ENABLED(BIG_FUNCTION_PTR))
                         {
                             if (GET_MODE(operands[0]) != HImode) abort();

                             if (hard_register_operand(operands[0], HImode))
                                     return "st\t%b0,%9\n\tst\t%t0,%8\n"
                                            "\tld\t%3,%8\n\tbsr\t%9";

                             else if(TEST_HARD_REG_BIT(reg_class_contents[FRAME_REGS],REGNO(operands[0])))
                                 return "ld\t%3,%t0\n\tbsr\t%b0";

                             else
                                 return "ld\t%4,%t0\n\tst\t%4,%8\n"
                                        "\tld\t%4,%b0\n\tst\t%4,%9\n"
                                        "\tld\t%3,%8\n\tbsr\t%9";
                         }
                         else
                         {
                             switch(REGNO(operands[0]))
                             {
                                 case RAH:
                                 case RAL:
                                     return "st\t%0,%9\n\tbsr\t%9";
                                 case RXL:
                                     return "bsr\t0,%4";
                                 default:
                                     return "bsr\t%0";
                             }
                         }
                         break;
        default:
                         abort();
    }
}

/* Prepare the operands for a comparison.  */

int xap2plus_expand_cmpqi(rtx *operands)
{
    rtx acc = force_reg(QImode, operands[0]);
    emit_insn (gen_rtx_SET (VOIDmode,
                cc0_rtx,
                gen_rtx_COMPARE (VOIDmode,
                    acc,
                    operands[1])));
    return 1;
}

int xap2plus_expand_cmphi(rtx *operands)
{
    rtx acc = gen_reg_rtx(HImode);
    emit_insn (gen_rtx(SET, VOIDmode, acc, operands[0]));
    emit_insn (gen_rtx_SET (VOIDmode,
                cc0_rtx,
                gen_rtx_COMPARE (VOIDmode,
                    acc,
                    operands[1])));
    return 1;
}

int xap2plus_expand_compare_reg (int relational, void *destination)
{
    enum rtx_code code = relational;
    rtx lab = (rtx)destination;
#ifdef HAVE_cc0
    rtx xap_flags = cc0_rtx;
#else
    rtx xap_flags = gen_rtx (REG, CCmode, XAP_FLAGS);
#endif
    rtx data_reg = gen_rtx(REG, HImode, RAH);
    int my_mode0, my_mode1;
    int my_mode = VOIDmode;
    int cmpmode = VOIDmode;
    int inverted = 0;
    rtx cond;
    rtx xap_compare_op0, xap_compare_op1, op0, op1;
    rtx old_seq = get_insns();

    /* !!!! */
    return 0;

    my_mode0 = GET_MODE(op0);
    my_mode1 = GET_MODE(op1);

    if ((my_mode0 == VOIDmode) && (my_mode1 == VOIDmode)) abort();
    else if (my_mode0 != VOIDmode)
        my_mode = my_mode0;
    else my_mode = my_mode1;
    cmpmode = my_mode;

    switch (code)
    {
        case EQ:
            inverted = 0;
            cmpmode = CCmode;
            op0 = xap_compare_op0;
            op1 = xap_compare_op1;
            cond = gen_rtx_EQ (VOIDmode, xap_flags, const0_rtx);
            break;

        case NE:
            inverted = 0;
            cmpmode = CCmode;
            op0 = xap_compare_op0;
            op1 = xap_compare_op1;
            cond = gen_rtx_NE (VOIDmode, xap_flags, const0_rtx);
            break;

        case LE:
            inverted = 1;
            op1 = xap_compare_op0;
            op0 = xap_compare_op1;
            code = LT;
            cond = gen_rtx_LT (VOIDmode, xap_flags, const0_rtx);
            break;

        case GT:
            inverted = 0;
            op1 = xap_compare_op0;
            op0 = xap_compare_op1;
            code = LT;
            cond = gen_rtx_LT (VOIDmode, xap_flags, const0_rtx);
            break;

        case GE:
            /* Use inverted condition, cmplt  */
            inverted = 1;
            op0 = xap_compare_op0;
            op1 = xap_compare_op1;
            code = LT;
            cond = gen_rtx_LT (VOIDmode, xap_flags, const0_rtx);
            break;

        case LT:
            inverted = 0;
            op0 = xap_compare_op0;
            op1 = xap_compare_op1;
            cond = gen_rtx_LT (VOIDmode, xap_flags, const0_rtx);
            break;

        case GTU:
            inverted = 0;
            op1 = xap_compare_op0;
            op0 = xap_compare_op1;
            code = LTU;
            cond = gen_rtx_LTU (VOIDmode, xap_flags, const0_rtx);
            break;

        case LEU:
            inverted = 1;
            op1 = xap_compare_op0;
            op0 = xap_compare_op1;
            code = LTU;
            cond = gen_rtx_LTU (VOIDmode, xap_flags, const0_rtx);
            break;

        case LTU:
            inverted = 0;
            op0 = xap_compare_op0;
            op1 = xap_compare_op1;
            cond = gen_rtx_LTU (VOIDmode, xap_flags, const0_rtx);
            break;

        case GEU:
            inverted = 1;
            op0 = xap_compare_op0;
            op1 = xap_compare_op1;
            code = LTU;
            cond = gen_rtx_LTU (VOIDmode, xap_flags, const0_rtx);
            break;

        default:
            abort();
            break;
    }

    if (CONSTANT_P (op1) && GET_CODE (op1) == CONST_INT && !INTVAL(op1))
    {
        data_reg = op0;
    }
    else
    {
        if (my_mode == HImode)
        {
            emit_move_insn(data_reg, op0);
        }
        else
        {
            data_reg = force_reg (my_mode, op0);
            cmpmode = CCmode;
        }
    }

    emit_insn_after (old_seq, gen_rtx_SET (VOIDmode,
                pc_rtx,
                inverted ? gen_rtx_IF_THEN_ELSE (VOIDmode,
                    cond,
                    pc_rtx,
                    gen_rtx_LABEL_REF (VOIDmode, lab)) 
                : gen_rtx_IF_THEN_ELSE (VOIDmode,
                    cond,
                    gen_rtx_LABEL_REF (VOIDmode, lab), 
                    pc_rtx)
                ));

    xap_compare_op0 = op0;
    xap_compare_op1 = op1;

    return 1;
}

static void xap2_plus_pr_pagezero_on(cpp_reader *dummy ATTRIBUTE_UNUSED)
{
    pagezero = 1;
}

static void xap2_plus_pr_pagezero_off(cpp_reader *dummy ATTRIBUTE_UNUSED)
{
    pagezero = 0;
}

void register_target_pragmas
#ifdef GCC_3P4
(void)
#else
(void *PFILE)
#endif
{
#ifdef GCC_3P4
    c_register_pragma (0, "pagezero_on", xap2_plus_pr_pagezero_on);
    c_register_pragma (0, "pagezero_off", xap2_plus_pr_pagezero_off);
#else
    cpp_register_pragma (PFILE, 0, "pagezero_on", xap2_plus_pr_pagezero_on);
    cpp_register_pragma (PFILE, 0, "pagezero_off", xap2_plus_pr_pagezero_off);
#endif
}
int xap_mode_dependent_address(void *addr)
{
    rtx ADDR = (rtx)addr;
    switch(GET_CODE(ADDR))
    {
        case POST_INC: return 1;
        case PRE_DEC: return 1;
        default: return 0;
    }
}

int hard_regno_nregs(int regno ATTRIBUTE_UNUSED, int mode)
{
    return ((GET_MODE_SIZE(mode) + UNITS_PER_WORD - 1) / UNITS_PER_WORD);
}

int truly_noop_truncation(
        int outprec ATTRIBUTE_UNUSED, int inprec ATTRIBUTE_UNUSED
                         )
{
    return 1;
}

int reg_set_or_referenced_p (reg, from_insn)
    rtx reg, from_insn;
{
    rtx insn, to_insn = get_last_insn();

    if (from_insn == to_insn)
        return 0;

    for (insn = from_insn; insn != to_insn; insn = NEXT_INSN (insn))
    {
        if (INSN_P (insn) && !quiet_flag)
        {
            print_rtx_head = ";";
            print_rtl_single (asm_out_file, insn);
            print_rtx_head = "";
        }
        if (GET_CODE (insn) == JUMP_INSN)
            return 0;
        if (INSN_P (insn) && reg_set_p (reg, insn))
            return 1;
        if (INSN_P (insn)
                && (reg_referenced_p (reg, PATTERN (insn))
                    || (GET_CODE (insn) == CALL_INSN
                        && find_reg_fusage (insn, USE, reg))))
            return 0;
    }
    return 0;
}

enum reg_class preferred_reload_class(
        rtx X ATTRIBUTE_UNUSED, enum reg_class class
                                     )
{
    return class;
}

int xap2plus_expand_movqi(rtx *operands)
{
    if (!quiet_flag)
    {
        if (reload_in_progress)
            fprintf(stderr, "reload xap2plus_expand_movqi(rtx, rtx)\n");
        else
            fprintf(stderr, "xap2plus_expand_movqi(rtx, rtx)\n");
        debug_rtx(operands[0]);
        debug_rtx(operands[1]);
    }
    if (reload_in_progress || reload_completed || no_new_pseudos)
    {
        if (   (GET_CODE(operands[1])==SUBREG) 
            && (    (GET_CODE(operands[0])!=REG)
                 || ((REGNO(operands[0]))>=RSPECIAL) )
           )
        {
            rtx temp = gen_rtx_REG(QImode, SUBREG_BYTE(operands[1]));

            emit_insn (                    gen_rtx (SET, VOIDmode, temp,
                        operands[1]));
            emit_insn (
                    gen_rtx (SET, VOIDmode, operands[0],
                        temp));
            return 1;
        }
        else if (    (GET_CODE(operands[0])==SUBREG) 
                  && (    (GET_CODE(operands[1])!=REG)
                       || ((REGNO(operands[1]))>=RSPECIAL))
                )
        {
            rtx temp = gen_rtx_REG(QImode, SUBREG_BYTE(operands[0]));

            emit_insn (                    gen_rtx (SET, VOIDmode, temp,
                        operands[1]));
            emit_insn (
                    gen_rtx (SET, VOIDmode, operands[0],
                        temp));
            return 1;
        }
        else if((GET_CODE(operands[1])==REG) && ((REGNO(operands[1]))<RSPECIAL))
            return 0;    
        else if((GET_CODE(operands[0])==REG) && ((REGNO(operands[0]))<RSPECIAL))
            return 0;        
        else abort();
    }
    else
    {
        if (    (    (GET_CODE(operands[1])==CONST_INT)
                  || (GET_CODE(operands[1])==SYMBOL_REF) )
             && (GET_CODE(operands[0])!=REG)
           )
        {
            rtx temp = gen_reg_rtx(QImode);

            emit_insn (                    gen_rtx (SET, VOIDmode, temp,
                        operands[1]));
            emit_insn (
                    gen_rtx (SET, VOIDmode, operands[0],
                        temp));
        }
        else if ((GET_CODE(operands[0])==MEM) && (GET_CODE(operands[1])!=REG))
        {
            rtx temp = gen_reg_rtx(QImode);

            emit_insn (                    gen_rtx (SET, VOIDmode, temp,
                        operands[1]));
            emit_insn (
                    gen_rtx (SET, VOIDmode, operands[0],
                        temp));
        }
        else
        {
            emit_insn (                    gen_rtx (SET, VOIDmode, operands[0],
                        operands[1]));
        }
        return 1;
    }
    return 0;
}

int xap2plus_expand_movhi(rtx *operands)
{
    if (!quiet_flag)
    {
        if (reload_in_progress)
            fprintf(stderr, "reload xap2plus_expand_movhi(rtx, rtx)\n");
        else
            fprintf(stderr, "xap2plus_expand_movhi(rtx, rtx)\n");
        debug_rtx(operands[0]);
        debug_rtx(operands[1]);
    }
    if (reload_in_progress || reload_completed || no_new_pseudos)
    {
        if (   !((GET_CODE(operands[1])==REG) && ((REGNO(operands[1]))<RSPECIAL))
            && !((GET_CODE(operands[0])==REG) && ((REGNO(operands[0]))<RSPECIAL)) )
        {
            rtx temp = gen_rtx_REG(HImode, RAH);

            emit_insn( gen_rtx(SET, VOIDmode, temp, operands[1]) );
            emit_insn( gen_rtx(SET, VOIDmode, operands[0], temp) );

            return 1;

        } /* else fall through to return 0 below */
    }
    else if(GET_CODE(operands[1])==CONST_INT)
    {
        /* We get better code if we split this up into individual 16 bit ops.
         * Incidentally it also avoids a bug where gcc inserts a move at the
         * beginnging of a function which could potentially stomp on function
         * args if we used AH and AL to do the move. */
        rtx temp_reg = gen_reg_rtx(QImode);
        rtx dest_msw = simplify_gen_subreg(QImode, operands[0], HImode, 0);
        rtx dest_lsw = simplify_gen_subreg(QImode, operands[0], HImode, 1);
        rtx src_msw = simplify_gen_subreg(QImode, operands[1], HImode, 0);
        rtx src_lsw = simplify_gen_subreg(QImode, operands[1], HImode, 1);
        
        emit_insn(gen_rtx(SET, VOIDmode, temp_reg, src_lsw));
        emit_insn(gen_rtx(CLOBBER, VOIDmode, operands[0]));
        emit_insn(gen_rtx(SET, VOIDmode, dest_lsw, temp_reg));
        if(INTVAL(src_lsw)!=INTVAL(src_msw))
        {
            rtx temp_reg_2 = gen_reg_rtx(QImode);
            emit_insn(gen_rtx(SET, VOIDmode, temp_reg_2, src_msw));
            emit_insn(gen_rtx(SET, VOIDmode, dest_msw, temp_reg_2));
        }
        else
        {
            emit_insn(gen_rtx(SET, VOIDmode, dest_msw, temp_reg));
        }
        return 1;
    }
    else
    {
        rtx temp = XAP_FLAG_ENABLED(TEST4) ? gen_rtx_REG(HImode, RAH)
                                : gen_reg_rtx(HImode);

        emit_insn( gen_rtx( SET, VOIDmode, temp, operands[1] ) );
        emit_insn( gen_rtx( SET, VOIDmode, operands[0], temp ) );
        return 1;
    }
    return 0;
}

int xap2plus_expand_reload_inqi(rtx *operands)
{
    if (reload_in_progress)
    {
        if (!quiet_flag)
        {
            fprintf(stderr, "xap2plus_expand_reload_inqi(rtx, rtx, rtx)\n");
            debug_rtx(operands[0]);
            debug_rtx(operands[1]);
            debug_rtx(operands[2]);
        }
        if (rtx_equal_p(operands[0],operands[1]))
            return 1;

        if (
               ((GET_CODE(operands[1])==REG) && ((REGNO(operands[1]))<RSPECIAL))
            && ((GET_CODE(operands[0])==REG) && ((REGNO(operands[0]))<RSPECIAL))
            && !XAP_FLAG_ENABLED(ENHANCED)
           )
        {
            if ((GET_CODE(operands[2])==REG) && ((REGNO(operands[2]))<RSPECIAL))
                abort();

            emit_insn (
                    gen_rtx (SET, QImode, operands[2], operands[1])
                      );
            emit_insn (
                    gen_rtx (SET, QImode, operands[0], operands[2])
                      );
            return 1;
        }

        if ((GET_CODE(operands[1])==REG) && ((REGNO(operands[1]))<RSPECIAL))
        {
            emit_insn (gen_rtx (SET, QImode, operands[0], operands[1]));
            return 1;
        }
        if ((GET_CODE(operands[0])==REG) && ((REGNO(operands[0]))<RSPECIAL))
        {
            emit_insn (gen_rtx (SET, QImode, operands[0], operands[1]));
            return 1;
        }

        emit_insn (gen_rtx (SET, QImode, operands[2], operands[1]));
        if (!rtx_equal_p(operands[0],operands[2]))
            emit_insn ( gen_rtx (SET, QImode, operands[0], operands[2]));
        return 1;

    }
    else
    {
        abort();
    }
}

int xap2plus_expand_reload_outqi(rtx *operands)
{
    if (reload_in_progress)
    {
        if (!quiet_flag)
        {
            fprintf(stderr, "xap2plus_expand_reload_outqi(rtx, rtx, rtx)\n");
            debug_rtx(operands[0]);
            debug_rtx(operands[1]);
            debug_rtx(operands[2]);
        }
        if (rtx_equal_p(operands[0],operands[1]))
            return 1;

        if (
               ((GET_CODE(operands[1])==REG) && ((REGNO(operands[1]))<RSPECIAL))
            && ((GET_CODE(operands[0])==REG) && ((REGNO(operands[0]))<RSPECIAL))
           )
        {
            if ((GET_CODE(operands[2])==REG) && ((REGNO(operands[2]))<RSPECIAL))
                abort();

            emit_insn (                    gen_rtx (SET, QImode, operands[2],
                        operands[1]));
            emit_insn (
                    gen_rtx (SET, QImode, operands[0],
                        operands[2]));
            return 1;
        }

        if ((GET_CODE(operands[1])==REG) && ((REGNO(operands[1]))<RSPECIAL))
        {
            emit_insn (                    gen_rtx (SET, QImode, operands[0],
                        operands[1]));
            return 1;
        }
        if ((GET_CODE(operands[0])==REG) && ((REGNO(operands[0]))<RSPECIAL))
        {
            emit_insn (                    gen_rtx (SET, QImode, operands[0],
                        operands[1]));

            return 1;
        }

        emit_insn (                    gen_rtx (SET, QImode, operands[2],
                    operands[1]));
        if (!rtx_equal_p(operands[0],operands[2]))
            emit_insn (
                    gen_rtx (SET, QImode, operands[0],
                        operands[2]));
        return 1;
    }
    else
    {
        abort();
    }
}

int xap2plus_expand_reload_inhi(rtx *operands)
{
    if (reload_in_progress)
    {
        if (!quiet_flag)
        {
            fprintf(stderr, "xap2plus_expand_reload_inhi(rtx, rtx, rtx)\n");
            debug_rtx(operands[0]);
            debug_rtx(operands[1]);
            debug_rtx(operands[2]);
        }
    }
    emit_insn (
            gen_rtx (SET, HImode, operands[0],
                operands[1]));
    return 1;
}

int xap2plus_expand_reload_outhi(rtx *operands)
{
    if (reload_in_progress)
    {
        if (!quiet_flag)
        {
            fprintf(stderr, "xap2plus_expand_reload_outhi(rtx, rtx, rtx)\n");
            debug_rtx(operands[0]);
            debug_rtx(operands[1]);
            debug_rtx(operands[2]);
        }
    }
    emit_insn (
            gen_rtx (SET, HImode, operands[0],
                operands[1]));
    return 1;
}

int xap2plus_expand_qi3(RTX_CODE operator, rtx *operands)
{
    if(!quiet_flag)
    {
        fprintf(stderr,"xap2plus_expand_qi3 %s%s\n",
                GET_RTX_NAME(operator),
                (reload_in_progress || reload_completed || no_new_pseudos)?" (reload)":"");
        debug_rtx(operands[0]);
        debug_rtx(operands[1]);
    }
    if (reload_in_progress || reload_completed || no_new_pseudos)
    {
        if (hard_register_operand(operands[0],QImode))
        {

        }
        else abort();
    }
    else
    {
        if (GET_CODE(operands[0])!=REG)
        {
            rtx temp = gen_reg_rtx(QImode);

            emit_insn (                    gen_rtx (SET, VOIDmode, temp,
                        operands[1]));
            emit_insn (gen_rtx_SET (VOIDmode,
                        temp,
                        gen_rtx_fmt_ee(operator, QImode,
                            temp,
                            operands[2])));

            emit_insn (
                    gen_rtx (SET, VOIDmode, operands[0],
                        temp));
        }
        else if (rtx_equal_p(operands[0],operands[1]))
        {
            emit_insn (gen_rtx_SET (VOIDmode,
                        operands[0],
                        gen_rtx_fmt_ee(operator, QImode,
                            operands[1],
                            operands[2])));
        }
        else
        {
            emit_insn (                    gen_rtx (SET, VOIDmode, operands[0],
                        operands[1]));
            emit_insn (gen_rtx_SET (VOIDmode,
                        operands[0],
                        gen_rtx_fmt_ee(operator, QImode,
                            operands[0],
                            operands[2])));
        }
        return 1;
    }
    return 0;
}

int xap2plus_expand_hi3(RTX_CODE operator, rtx *operands)
{
    if (reload_in_progress || reload_completed || no_new_pseudos)
    {
        if (!hard_register_operand(operands[0],HImode)) abort();
        return 0;
    }
    else
    {
        rtx temp = (XAP_FLAG_ENABLED(ENHANCED)) ? gen_reg_rtx(HImode)
                                     : gen_rtx_REG(HImode,RAH);

        emit_insn(gen_rtx_SET(VOIDmode, temp, operands[1]));
        emit_insn(gen_rtx_SET(VOIDmode,
                              temp,
                              gen_rtx_fmt_ee(operator, HImode,
                                             temp, operands[2])));
        emit_insn(gen_rtx_SET(VOIDmode, operands[0], temp));
        return 1;
    }
}

int xap2plus_expand_plus_hi3(RTX_CODE operator, rtx *operands)
{
    emit_insn (
     gen_rtx_SET(VOIDmode,
                 operands[0],
                 gen_rtx_fmt_ee(operator, HImode, operands[1], operands[2]))
              );
    return 1;
}

int xap2plus_expand_shift_qi3(
        RTX_CODE operator, rtx *operands
                             )
{
    rtx shift_reg, clobber_reg;
    rtx al = gen_rtx_REG(QImode, RAL);
    rtx ah = gen_rtx_REG(QImode, RAH);
    rtx body = gen_rtx_PARALLEL (VOIDmode, rtvec_alloc(2));

    /* Perform `<<' in AL and `>>' in AH */
    if(operator==ASHIFT)    { shift_reg = al; clobber_reg = ah; }
    else                    { shift_reg = ah; clobber_reg = al; }

    /* Doing op1>>op2 for constant op1, we can use an asr rather than a lsr in
     * the case that the top bit of op1 is not set, saving one instruction
     * (since lsr needs a prefix) */
    if(   operator==LSHIFTRT
       && GET_CODE(operands[1])==CONST_INT
       && !(INTVAL(operands[1]) & 0x8000)) operator = ASHIFTRT;

    XVECEXP(body, 0, 0) =
        gen_rtx(SET, VOIDmode, shift_reg, gen_rtx_fmt_ee(operator, QImode, shift_reg, operands[2]));
    XVECEXP(body, 0, 1) =
        gen_rtx_fmt_e(CLOBBER, QImode, clobber_reg);

    emit_insn(gen_rtx(SET, VOIDmode, shift_reg, operands[1]));
    emit_insn(body);
    emit_insn(gen_rtx(SET, VOIDmode, operands[0], shift_reg));

    return 1;
}

int xap2plus_expand_shift_hi3(
        RTX_CODE operator, rtx *operands
                             )
{
    rtx ah = gen_rtx_REG(HImode, RAH);

    /* Doing op1>>op2 for constant op1, we can use an asr rather than a lsr in
     * the case that the top bit of op1 is not set, saving one instruction
     * (since lsr needs a prefix) */
    if(   operator==LSHIFTRT
       && GET_CODE(operands[1])==CONST_INT
       && !(INTVAL(operands[1]) & 0x80000000)) operator = ASHIFTRT;

    emit_insn(gen_rtx(SET, VOIDmode, ah, operands[1]));
    emit_insn(gen_rtx(SET, VOIDmode, ah, gen_rtx_fmt_ee(operator, HImode, ah, operands[2])));
    emit_insn(gen_rtx(SET, VOIDmode, operands[0], ah));

    return 1;
}

int xap2plus_expand_mult_qi3(
        RTX_CODE operator, RTX_CODE extend_operator, rtx *operands
                            )
{
    if (reload_in_progress || reload_completed || no_new_pseudos)
    {
        if (hard_register_operand(operands[0],QImode))
        {

        }
        else abort();
    }
    else
    {
        rtx temp1 = gen_rtx_REG(HImode, RAH);
        rtx temp2 = gen_rtx_REG(QImode, RAL);
        rtx temp3;
        rtx shift = (GET_CODE(operands[2])==CONST_INT) ?
            operands[2] : 
            gen_rtx_fmt_e (extend_operator, HImode, operands[2]);

        emit_insn ( gen_rtx (SET, VOIDmode, temp2, operands[1]));

        temp3 = gen_rtx_fmt_ee(operator, HImode,
                gen_rtx_fmt_e (extend_operator, HImode, temp2),
                shift);

        if (GET_MODE(operands[0])==QImode)
        {
            rtx body = gen_rtx_PARALLEL (VOIDmode, rtvec_alloc(2));
            XVECEXP(body, 0, 0) =
                gen_rtx (SET, VOIDmode, temp2,
                         simplify_gen_subreg(QImode, temp3, HImode, 1));
            XVECEXP(body, 0, 1) =
                gen_rtx_fmt_e(CLOBBER, QImode, gen_rtx_REG(QImode, RAH));
            emit_insn(body);
            emit_insn(gen_rtx (SET, VOIDmode, operands[0], temp2));
        }
        else
        {
            emit_insn ( gen_rtx (SET, VOIDmode, temp1, temp3));
            emit_insn ( gen_rtx (SET, VOIDmode, operands[0], temp1));
        }

        return 1;
    }
    return 0;
}

int xap2plus_expand_div_qi3(
        RTX_CODE operator, RTX_CODE extend_operator, rtx *operands
                           )
{
    if (reload_in_progress || reload_completed || no_new_pseudos)
    {
        if (hard_register_operand(operands[0],QImode))
        {

        }
        else abort();
    }
    else
    {
        rtx temp1 = gen_rtx_REG(HImode, RAH);
        rtx temp2 = gen_rtx_REG(QImode, RAL);
        rtx temp3 = gen_rtx_REG(QImode, RAH);
        rtx temp4 = (operator == DIV) || (operator == UDIV) ? temp2 : temp3;
        rtx shift = (GET_CODE(operands[2])==CONST_INT) ?
            operands[2] : 
            gen_rtx_fmt_e (extend_operator, HImode, operands[2]);

        emit_insn(
           gen_rtx(SET, VOIDmode, temp1, 
                   gen_rtx_fmt_e(extend_operator, HImode, operands[1]))
                 );

        emit_insn (gen_rtx_SET (VOIDmode,
                    temp4,
                    simplify_gen_subreg(QImode,
                        gen_rtx_fmt_ee(operator, HImode,
                            temp1,
                            shift), HImode, 1)));

        emit_insn ( gen_rtx (SET, VOIDmode, operands[0], temp4));

        return 1;
    }
    return 0;
}

int xap2plus_expand_arith_hi3(RTX_CODE operator, rtx *operands)
{
    const char *libname;
    if(!quiet_flag)
    {
        fprintf(stderr,"xap2plus_expand_arith_hi3 operator = %s\n",GET_RTX_NAME(operator));
        debug_rtx(operands[0]);
        debug_rtx(operands[1]);
        debug_rtx(operands[2]);
    }
    switch(operator)
    {
        case UDIV: libname = "__udivhi3"; break;
        case UMOD: libname = "__umodhi3"; break;
        case DIV:  libname = "__divhi3"; break;
        case MOD:  libname = "__modhi3"; break;
        case MULT:
                   {
                       if (GET_CODE(operands[2])==CONST_INT)
                       {
                           int shift = exact_log2(INTVAL(operands[2]));
                           if (shift >= 0)
                           {
                               emit_insn (gen_rtx_SET (VOIDmode,
                                           gen_rtx_REG (HImode,
                                               0),
                                           operands[1]));
                               emit_insn (gen_rtx_SET (VOIDmode,
                                           gen_rtx_REG (HImode,
                                               0),
                                           gen_rtx_ASHIFT (HImode,
                                               gen_rtx_REG (HImode,
                                                   0),
                                               GEN_INT(shift))));
                               emit_insn (gen_rtx_SET (VOIDmode,
                                           operands[0],
                                           gen_rtx_REG (HImode,
                                               0)));
                               return 1;
                           }
                       }
                       libname = "__mulhi3"; 
                       break;
                   }
        default: abort();
    }

    emit_library_call_value(
            gen_rtx_SYMBOL_REF(Pmode, libname),
            operands[0],
            LCT_CONST,
            HImode,
            2,
            operands[1], HImode,
            operands[2], HImode
                           );

    return 1;
}

int xap2plus_expand_movstrqi(rtx *operands)
{
    emit_insn (gen_rtx_USE (VOIDmode, operands[0]));
    emit_insn (gen_rtx_USE (VOIDmode, operands[1]));
    emit_insn (gen_rtx_USE (VOIDmode, operands[2]));

    if (XAP_FLAG_ENABLED(ENHANCED))
    {
        operands[4] = gen_rtx_REG (Pmode, RAH);
        operands[5] = gen_rtx_REG (Pmode, RXL);
        operands[6] = gen_rtx_REG (QImode, RAL);
        emit_insn (gen_rtx_SET (Pmode, operands[4], XEXP (operands[0], 0)));
        emit_insn (gen_rtx_SET (VOIDmode, operands[6], operands[2]));
        emit_insn (gen_rtx_SET (Pmode, operands[5], XEXP (operands[1], 0)));
        emit_insn (gen_rtx_USE (VOIDmode, operands[6]));
        emit_insn (gen_rtx_USE (VOIDmode, operands[5]));
        emit_insn (gen_rtx_USE (VOIDmode, operands[4]));

        emit (gen_rtx_PARALLEL (VOIDmode, gen_rtvec (5,
                  GEN_INT (12346),
                  gen_rtx_CLOBBER (VOIDmode, operands[5]),
                  gen_rtx_SET (VOIDmode, operands[6], const0_rtx),
                  gen_rtx_CLOBBER (VOIDmode, gen_rtx_MEM (BLKmode, const0_rtx)),
                  gen_rtx_CLOBBER (VOIDmode, operands[4]))));
    }
    else
    {
        operands[4] = copy_to_mode_reg (Pmode, XEXP (operands[0], 0));
        operands[5] = copy_to_mode_reg (Pmode, XEXP (operands[1], 0));
        operands[6] = gen_rtx(REG, QImode, RSAVED_Y);
        operands[7] = gen_rtx(REG, QImode, RBC);
        operands[8] = gen_rtx_REG (QImode, RFP);

        emit_insn(gen_rtx_SET(VOIDmode, gen_rtx_REG(QImode, RAL), operands[2]));
        emit_insn(gen_rtx_SET (VOIDmode, operands[7], operands[5]));
        emit_insn(gen_rtx_SET (VOIDmode, operands[6], operands[8]));
        emit_insn(gen_rtx_SET (VOIDmode, operands[8], operands[4]));
        emit_insn(gen_rtx_SET(VOIDmode, gen_rtx_REG(QImode, RXL), operands[7]));
        emit_insn(gen_rtx_USE (VOIDmode, operands[6]));
        emit_insn(gen_rtx_USE (VOIDmode, gen_rtx_REG (QImode, RAL)));
        emit_insn(gen_rtx_USE (VOIDmode, gen_rtx_REG (QImode, RXL)));
        emit_insn(gen_rtx_USE (VOIDmode, operands[7]));

        emit (gen_rtx_PARALLEL (VOIDmode, gen_rtvec (6,
                    GEN_INT (12345),
                    gen_rtx_CLOBBER (VOIDmode, gen_rtx_REG (QImode, RXL)),
                    gen_rtx_SET(VOIDmode, gen_rtx_REG(QImode, RAL), const0_rtx),
                    gen_rtx_CLOBBER(VOIDmode, gen_rtx_MEM(BLKmode, const0_rtx)),
                    gen_rtx_SET (VOIDmode, operands[8], operands[6]),
                    gen_rtx_CLOBBER (VOIDmode, operands[6]))));
    }
    return 1;
}

/* dummy functions to reconcile typedefs */

tree xap_tree_inlining_walk_subtrees(
            tree *_1, int *_2, void *_3, void *_4, void *_5
                                    )
{
    extern tree lhd_tree_inlining_walk_subtrees(
            tree *_1, int *_2, walk_tree_fn _3, void *_4, void *_5
                                               );
    return lhd_tree_inlining_walk_subtrees(_1, _2, (walk_tree_fn)_3, _4, _5);
}

#ifdef GCC_3P4
size_t xap_tree_size(enum xap_tree_code _1)
{
    extern size_t lhd_tree_size(enum tree_code);
    return lhd_tree_size(_1);
}
#endif

/* Check for special XAP pointer conversions. Otherwise do nothing (return the
 * tree node passed).  */

tree xap_lang_hooks_expand_constant(tree value)
{
    xap_thissize = int_size_in_bytes (TREE_TYPE (value));
    switch (TREE_CODE(value))
    {
        case CONVERT_EXPR:
        case NOP_EXPR:
            {
                tree t0 = TREE_OPERAND (value, 0);
                if (TREE_CODE(t0)==CONVERT_EXPR)
                {
                    t0 = TREE_OPERAND (t0, 0);
                }
                /* Allow (int) &foo provided int is as wide as a pointer.  */
                if ((INTEGRAL_TYPE_P (TREE_TYPE (value)) || POINTER_TYPE_P (TREE_TYPE (value)))
                        && POINTER_TYPE_P (TREE_TYPE (t0)))

                    switch(TREE_CODE(t0))
                    {
                        case ADDR_EXPR:
                            {
                                tree t1 = TREE_OPERAND(t0,0);
                                switch(TREE_CODE(t1))
                                {
                                    case FUNCTION_DECL:
                                    case VAR_DECL:
                                        {
                                            rtx lnk0 = DECL_RTL_IF_SET(t1);
                                            if (lnk0) switch (GET_CODE(lnk0))
                                            {
                                                case MEM:
                                                    {
                                                        rtx lnk1 = XEXP(lnk0,0);
                                                        switch (GET_CODE(lnk1))
                                                        {
                                                            case SYMBOL_REF:
                                                                {
                                                                    int dest_prec = POINTER_TYPE_P (TREE_TYPE (value)) ?
                                                                        TYPE_PRECISION (TREE_TYPE(TREE_TYPE (value))) : 
                                                                        TYPE_PRECISION (TREE_TYPE (value));
                                                                    int src_prec = TYPE_PRECISION (TREE_TYPE (t0));
                                                                    if (dest_prec >= src_prec)
                                                                    {
                                                                        if (!TREE_LANG_FLAG_0(t1))
                                                                        {
                                                                            TREE_LANG_FLAG_0(t1) = 1;
                                                                            pedwarn (
                                                                                    "converting initializer element %s of type %s to %s type is not portable", 
                                                                                    XSTR(lnk1,0), 
                                                                                    TREE_CODE(t1)==FUNCTION_DECL?"function pointer":"address of variable", 
                                                                                    POINTER_TYPE_P (TREE_TYPE (value))?"pointer":"integer");
                                                                        }
                                                                        return t0;
                                                                    }
                                                                }
                                                                break;
                                                            default:
                                                                break;
                                                        }
                                                    }
                                                    break;
                                                default:
                                                    break;
                                            }
                                        }
                                        break;
                                    default:
                                        break;
                                }
                            }
                            break;
                        default:
                            break;
                    }
                break;
                default:
                break;
            }
    }
    return value;
}

tree decl = NULL;
tree type = NULL;

static int xap_tree_code(tree local_decl, tree local_type)
{
    if ((local_decl != decl) && (local_type != type)) abort();
    if (local_decl)
    {
        if (TREE_CODE(local_decl)==FUNCTION_DECL)
            return XAP_FLAG_ENABLED(BIG_FUNCTION_PTR);
    }
    if (local_type)
    {
        if ((TREE_CODE(local_type) == POINTER_TYPE) && 
                (TREE_CODE(TREE_TYPE(local_type)) == FUNCTION_TYPE))
        {
            return XAP_FLAG_ENABLED(BIG_FUNCTION_PTR);
        }
    }
    return 0;
}

enum machine_mode xap_pmode(tree local_decl, tree local_type, const char * calling_file, int calling_line )
{
    int code = xap_tree_code(local_decl, local_type);
    if (!quiet_flag)
        fprintf(
               stderr,
               "       xap_pmode code=%-8d decl:%s type:%s called from %s:%d\n",
               code,
               (local_decl)?"not null":"null    ",
               (local_type)?"not null":"null    ",
               calling_file,
               calling_line
               );
    return code ? HImode : QImode;
}

int xap_pointer_size(tree local_decl, tree local_type, const char * calling_file, int calling_line)
{
    int code = xap_tree_code(local_decl, local_type);
    if (!quiet_flag)
        fprintf(
               stderr,
               "xap_pointer_size code=%-8d decl:%s type:%s called from %s:%d\n",
               code,
               (local_decl)?"not null":"null    ",
               (local_type)?"not null":"null    ",
               calling_file,
               calling_line
               );
    return code ? 32 : 16;
}

void xap_lang_hooks_function_init (f)
    struct function *f ATTRIBUTE_UNUSED;
{
    if (current_function_decl)
    {
        rtx lnk0 = DECL_RTL_IF_SET(current_function_decl);
        if (lnk0) switch (GET_CODE(lnk0))
        {
            case MEM:
                {
                    rtx lnk1 = XEXP(lnk0,0);
                    switch (GET_CODE(lnk1))
                    {
                        case SYMBOL_REF:
                            {
                                if (!XAP_FLAG_ENABLED(QUIET))
                                    fprintf(asm_out_file,
                                            "%s Function %s\n",
                                            asm_comment_start(), XSTR(lnk1,0));
                            }
                        default:
                            break;
                    }
                }
            default:
                break;
        }
    }
}

void xap_lang_hooks_function_final (f)
    struct function *f ATTRIBUTE_UNUSED;
{
}

void asm_output_skip(FILE *f, int n)
{
    n -= xap_excess_size;
    xap_excess_size = 0;
    if (xap_excess_size)
    {
        fprintf(f,
                "%s size reduced by %d to %d\n",
                asm_comment_start(), xap_excess_size, n);
    }
    xap_object_size += n;

    if(in_bss_section())
        fprintf(f,"\tDS\t%d\n",n);
    else
        while(n--) fprintf(f, "\tdc\tH'0000\n");
}

void xap2plus_cpu_cpp_builtins(void *pfile, builtin_define_t target_define)
{
    target_define (pfile, "XAP");
    target_define (pfile, "__XAP__");
    if (!XAP_FLAG_ENABLED(XAP2)) target_define (pfile, "__BLUELAB__");
}

enum xap_dir {INPUT, OUTPUT};

static enum reg_class xap_secondary_reload_class(
        enum reg_class class, enum machine_mode MODE, rtx IN, enum xap_dir DIR
                                                )
{
    enum reg_class retval = NO_REGS;
    int addr = (GET_CODE(IN)==MEM) && legitimate_address(MODE, XEXP(IN,0), 1);
    int hard = hard_register_operand(IN, MODE);
    if (!quiet_flag)
        fprintf(
                stderr,
"xap_secondary_reload_class(enum reg_class %s, enum machine_mode %d, legit %d, hard %d)\n",
                reg_class_names[class], MODE, addr, hard
               );
    switch(class)
    {
        case ADDR_REGS:
            retval = NO_REGS;
            break;
        case DATA_REGS:
        case CHIP_REGS: 
            retval =   addr ? NO_REGS 
                            : ( hard ? (XAP_FLAG_ENABLED(ENHANCED) ? NO_REGS : FAKE_REGS)
                            : (DIR==INPUT ? NO_REGS : ADDR_REGS) ); 
            break;
        case FAKE_REGS:
        case FRAME_REGS:
            retval = addr ? CHIP_REGS : (DIR==INPUT ? CHIP_REGS : ADDR_REGS);
            break;
        case NO_REGS:
            retval = NO_REGS;
            break;
        default: abort();
    }
    return retval;
}

enum reg_class xap_secondary_input_reload_class(
        enum reg_class class, enum machine_mode MODE, rtx IN
                                               )
{
    enum reg_class retval = NO_REGS;
    if (reload_in_progress)
    {
        retval = xap_secondary_reload_class(class, MODE, IN, INPUT);
        if (!quiet_flag)
        {
            fprintf(
                    stderr, 
                    "xap_secondary_input_reload_class(enum reg_class %s, enum machine_mode %d, rtx )\n",
                    reg_class_names[class], MODE
                   );
            debug_rtx(IN);
            fprintf(stderr, "retval = %s\n", reg_class_names[retval]);
        }
    }
    return retval;
}

enum reg_class xap_secondary_output_reload_class(
        enum reg_class class, enum machine_mode MODE, rtx IN
                                                )
{
    enum reg_class retval = NO_REGS;
    if (reload_in_progress)
    {
        retval = xap_secondary_reload_class(class, MODE, IN, OUTPUT);
        if (!quiet_flag)
        {
            fprintf(
                    stderr,
                    "xap_secondary_output_reload_class(enum reg_class %s, enum machine_mode %d, rtx )\n",
                    reg_class_names[class], MODE
                   );
            debug_rtx(IN);
            fprintf(stderr, "retval = %s\n", reg_class_names[retval]);
        }
    }
    return retval;
}

int xap_secondary_memory_needed(
            enum reg_class class1, enum reg_class class2, enum machine_mode MODE
                               )
{
    if (!quiet_flag)
    {
        fprintf(
                stderr,
                "xap_secondary_memory_needed(enum reg_class %s, enum reg_class %s, mode %d)\n",
                reg_class_names[class1],reg_class_names[class2], MODE
               );
    }
    if (XAP_FLAG_ENABLED(ENHANCED)) return 0;
    if ((class1==ADDR_REGS) && (class2==ADDR_REGS)) return 1;
    if ((class1==ADDR_REGS) && (class2==CHIP_REGS)) return 1;
    if ((class1==ADDR_REGS) && (class2==DATA_REGS)) return 1;
    if ((class1==ADDR_REGS) && (class2==FAKE_REGS)) return 0;
    if ((class1==ADDR_REGS) && (class2==FRAME_REGS)) return 0;
    if ((class1==CHIP_REGS) && (class2==ADDR_REGS)) return 1;
    if ((class1==CHIP_REGS) && (class2==CHIP_REGS)) return 1;
    if ((class1==CHIP_REGS) && (class2==DATA_REGS)) return 1;
    if ((class1==CHIP_REGS) && (class2==FAKE_REGS)) return 0;
    if ((class1==CHIP_REGS) && (class2==FRAME_REGS)) return 0;
    if ((class1==DATA_REGS) && (class2==ADDR_REGS)) return 1;
    if ((class1==DATA_REGS) && (class2==CHIP_REGS)) return 1;
    if ((class1==DATA_REGS) && (class2==DATA_REGS)) return 1;
    if ((class1==DATA_REGS) && (class2==FAKE_REGS)) return 0;
    if ((class1==DATA_REGS) && (class2==FRAME_REGS)) return 0;
    if ((class1==FAKE_REGS) && (class2==ADDR_REGS)) return 0;
    if ((class1==FAKE_REGS) && (class2==CHIP_REGS)) return 0;
    if ((class1==FAKE_REGS) && (class2==DATA_REGS)) return 0;
    if ((class1==FAKE_REGS) && (class2==FAKE_REGS)) return 0;
    if ((class1==FAKE_REGS) && (class2==FRAME_REGS)) return 0;
    if ((class1==FRAME_REGS) && (class2==ADDR_REGS)) return 0;
    if ((class1==FRAME_REGS) && (class2==CHIP_REGS)) return 0;
    if ((class1==FRAME_REGS) && (class2==DATA_REGS)) return 0;
    if ((class1==FRAME_REGS) && (class2==FAKE_REGS)) return 0;
    if ((class1==FRAME_REGS) && (class2==FRAME_REGS)) return 0;
    abort();
}

rtx xap_secondary_memory_needed_rtx(enum machine_mode MODE)
{
    return
        gen_rtx_MEM(MODE, gen_rtx_PLUS(QImode, frame_pointer_rtx, GEN_INT(-2)));
}

/* Output label and .stab for the end of the function */
void xap2plus_dbx_function_end(FILE *stream, tree func)
{
    fprintf(stream, "%s\"\",192,0,0,?LBB_%s\n",ASM_STABS_OP,IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(func)));
    fprintf(stream, "%s\"\",224,0,0,?LBE_%s\n",ASM_STABS_OP,IDENTIFIER_POINTER(DECL_ASSEMBLER_NAME(func)));
}

const char *xap_asm_stabs_op(void)
{
    if(XAP_FLAG_ENABLED(XAP2))
        return "\t.stabs\t";
    else
        return ";.stabs\t";
}

const char *xap_asm_stabd_op(void)
{
    if(XAP_FLAG_ENABLED(XAP2))
        return "\t.stabd\t";
    else
        return ";.stabd\t";
}

const char *xap_asm_stabn_op(void)
{
    if(XAP_FLAG_ENABLED(XAP2))
        return "\t.stabn\t";
    else
        return ";.stabn\t";
}
