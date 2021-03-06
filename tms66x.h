#ifndef _TMS66x_HPP
#define _TMS66x_HPP

#include <idp.hpp>
#include <idaidp.hpp>
#include "ins.hpp"
#include "fetch_packet.h"


// 汇编指令格式，一切flag和枚举皆为生成这样格式的汇编代码
// [parallel] [cond] ins .[unit][cross path] [op1], [op2], [op3]，
// parallel为并行指令符号，记录在aux_para中，cflags使用insn_t中的auxpref_u8[1]成员存储
// cond为指令执行的条件，当条件为假时，该指令等同于nop。cond使用c或cn开头的flag，使用insn_t中的auxpref_u8[0]成员存储
// ins为指令操作码，既有字符串形式，也有枚举形式，两者需一一对应，LPH也需要这两个定义，使用insn_t中的itype成员存储
// unit为指令操作的单元，为funit_t的枚举，使用insn_t中的segpref成员存储
// cross path是否存在寄存器路径交叉，记录在aux_xp中，cflags使用insn_t中的auxpref_u8[1]成员存储
// op为操作数，主要为立即数(o_imm)和寄存器(o_reg)。寄存器中存在寄存器对的概念，该类型记为o_regpair

//---------------------------------
// Operand types:
// 新增的操作数类型、ana中需要为op_t设置该值
#define o_regpair       o_idpspec0       // 64bits Register pair (A1:A0 B1:B0)
#define o_regqpair      o_idpspec1      // 128bits Register pair (A3:A2:A1:A0 B3:B2:B1:B0)
#define o_spmask        o_idpspec2      // unit mask (reg)
#define o_stgcyc        o_idpspec3      // fstg/fcyc (value)

// o_phrase: the second register is held in secreg (specflag1)
// baseR[offsetR]寻址方式中用于保存offsetR的值
#define secreg          specflag1   //op_t->specflag1
// 仅用于四个操作数时，其中的src2保存在op1.specflag2中
#define src2            specflag2   //op_t->specflag2

//------------------------------------------------------------------
// o_phrase, o_displ: mode
// 寻址方式，共12种，详见Table 3-11 Address Generator Options for Load/Store
#define mode            specflag3   //op_t->specflag2
enum mode_t : uint8_t
{
    MO_SUB_UCST = 0,        //*-R[ucst5]
    MO_ADD_UCST = 1,        //*+R[ucst5]
    MO_SUB_REG = 4,         //*-R[offsetR]
    MO_ADD_REG = 5,         //*+R[offsetR]
    MO_SUBSUB_UCST = 8,     //*--R[ucst5]
    MO_ADDADD_UCST = 9,     //*++R[ucst5]
    MO_UCST_SUBSUB = 10,    //*R--[ucst5]
    MO_UCST_ADDADD = 11,    //*R++[ucst5]
    MO_SUBSUB_REG = 12,     //*--R[offsetR]
    MO_ADDADD_REG = 13,     //*++R[offsetR]
    MO_REG_SUBSUB = 14,     //*R--[offsetR]
    MO_REG_ADDADD = 15,     //*R++[offsetR]
};

//------------------------------------------------------------------
// 指令的操作单元、ana中需要为insn设置该值
//NOTE: FU_L1=1  FU_M2=8 不能改变。在输出循环标记时写死了该值
// Functional units:
#define funit segpref     //insn_t->segpref
enum funit_t : uint8_t
{
  FU_NONE,                      // No unit (NOP, IDLE)
  FU_L1, FU_L2,                 // 32/40-bit arithmetic and compare operations
                                // Leftmost 1 or 0 bit counting for 32 bits
                                // Normalization count for 32 and 40 bits
                                // 32-bit logical operations

  FU_S1, FU_S2,                 // 32-bit arithmetic operations
                                // 32/40-bit shifts and 32-bit bit-field operations
                                // 32-bit logical operations
                                // Branches
                                // Constant generation
                                // Register transfers to/from the control register file (.S2 only)
  FU_D1, FU_D2,                 // 32-bit add, subtract, linear and circular address calculation
                                // Loads and stores with a 5-bit constant offset
                                // Loads and stores with 15-bit constant offset (.D2 only)
  FU_M1, FU_M2,                 // 16 x 16 bit multiply operations


};
//------------------------------------------------------------------
// condition codes:
// 条件flag，ana中需要为insn设置该值，详见Table 3-9 Registers That Can Be Tested by Conditional Operations
#define cond auxpref_u8[0]      // The condition code of instruction
enum cond_t : uint8_t
{
    CO_AL = 0x0,      // 0000 unconditional
    CO_B0 = 0x2,      // 0010 B0
    CO_NB0 = 0x3,     // 0011 !B0
    CO_B1 = 0x4,      // 0100 B1
    CO_NB1 = 0x5,     // 0101 !B1
    CO_B2 = 0x6,      // 0110 B2
    CO_NB2 = 0x7,     // 0111 !B2
    CO_A1 = 0x8,      // 1000 A1
    CO_NA1 = 0x9,     // 1001 !A1
    CO_A2 = 0xA,      // 1010 A2
    CO_NA2 = 0xB,     // 1011 !A2
    CO_A0 = 0xC,      // 1100 A0
    CO_NA0 = 0xD,     // 1101 !A0
    //Reserved = 1, 0xE, 0xF
};

//------------------------------------------------------------------
#define cflags auxpref_u8[1]      // Various bit definitions:
// 额外的cflags，ana中需要为insn设置该值，
#define aux_para      0x0001  // parallel execution with the next insn
#define aux_xp        0x0002  // X path is used
#define aux_src2      0x0004  // 是否为4个操作数的指令
#define aux_fph       0x0008  // fetch packet header
#define aux_pseudo    0x0010  // pseudo ins(伪指令)
#define aux_ldst      0x0020  // load和store指令
#define aux_t2        0x0040  // load和store指令是否使用T2

//------------------------------------------------------------------
enum RegNo : uint16_t
{
  rA0, rA1,  rA2, rA3,  rA4,  rA5,  rA6,  rA7,
  rA8, rA9, rA10, rA11, rA12, rA13, rA14, rA15,
  rA16, rA17, rA18, rA19, rA20, rA21, rA22, rA23,
  rA24, rA25, rA26, rA27, rA28, rA29, rA30, rA31,
  rB0, rB1, rB2,  rB3,  rB4,  rB5,  rB6,  rB7,
  rB8, rB9, rB10, rB11, rB12, rB13, rB14, rB15,
  rB16, rB17, rB18, rB19, rB20, rB21, rB22, rB23,
  rB24, rB25, rB26, rB27, rB28, rB29, rB30, rB31,
  rAMR,
  rCSR,
  rIFR,
  rISR,
  rICR,
  rIER,
  rISTP,
  rIRP,
  rNRP,
  rACR,
  rADR,
  rPCE1,
  rFADCR,
  rFAUCR,
  rFMCR,
  rTSCL,
  rTSCH,
  rILC,
  rRILC,
  rREP,
  rDNUM,
  rSSR,
  rGPLYA,
  rGPLYB,
  rGFPGFR,
  rTSR,
  rITSR,
  rNTSR,
  rECR,
  rEFR,
  rIERR,
  rVcs, rVds,            // virtual registers for code and data segments
};

extern int data_id;  //for SET_MODULE_DATA
struct tms66x_t : public procmod_t
{
    ssize_t idaapi on_event(ssize_t msgid, va_list va);
};

int idaapi ana16(insn_t* insn, fetch_packet_t* fp);
int idaapi ana32(insn_t* insn, fetch_packet_t* fp);
int idaapi emu(const insn_t* insn);
void idaapi out_insn(outctx_t& ctx);
void idaapi out_mnem(outctx_t& ctx);
bool idaapi out_opnd(outctx_t& ctx, const op_t& x);
void idaapi header(outctx_t* ctx);
void idaapi footer(outctx_t* ctx);
void idaapi check_func_bounds(int* possible_return_code, func_t* pfn, ea_t max_func_end_ea);
int idaapi is_align_insn(ea_t ea);
#endif // _TMS66x_HPP
