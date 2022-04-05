#ifndef _TMS66x_HPP
#define _TMS66x_HPP

#include <idp.hpp>
#include <idaidp.hpp>
#include "ins.hpp"
#include "fetch_packet.h"


// ���ָ���ʽ��һ��flag��ö�ٽ�Ϊ����������ʽ�Ļ�����
// [parallel] [cond] ins .[unit][cross path] [op1], [op2], [op3]��
// parallelΪ����ָ����ţ���¼��aux_para�У�cflagsʹ��insn_t�е�auxpref_u8[1]��Ա�洢
// condΪָ��ִ�е�������������Ϊ��ʱ����ָ���ͬ��nop��condʹ��c��cn��ͷ��flag��ʹ��insn_t�е�auxpref_u8[0]��Ա�洢
// insΪָ������룬�����ַ�����ʽ��Ҳ��ö����ʽ��������һһ��Ӧ��LPHҲ��Ҫ���������壬ʹ��insn_t�е�itype��Ա�洢
// unitΪָ������ĵ�Ԫ��Ϊfunit_t��ö�٣�ʹ��insn_t�е�segpref��Ա�洢
// cross path�Ƿ���ڼĴ���·�����棬��¼��aux_xp�У�cflagsʹ��insn_t�е�auxpref_u8[1]��Ա�洢
// opΪ����������ҪΪ������(o_imm)�ͼĴ���(o_reg)���Ĵ����д��ڼĴ����Եĸ�������ͼ�Ϊo_regpair

//---------------------------------
// Operand types:
// �����Ĳ��������͡�ana����ҪΪop_t���ø�ֵ
#define o_regpair       o_idpspec0      // Register pair (A1:A0..B15:B14)
                                        // Register pair is denoted by its
                                        // even register in op.reg
                                        // (Odd register keeps MSB)
#define o_spmask        o_idpspec1      // unit mask (reg)
#define o_stgcyc        o_idpspec2      // fstg/fcyc (value)

// o_phrase: the second register is held in secreg (specflag1)
// baseR[offsetR]Ѱַ��ʽ�����ڱ���offsetR��ֵ
#define secreg          specflag1   //op_t->specflag1
// �������ĸ�������ʱ�����е�src2������op1.specflag2��
#define src2            specflag2   //op_t->specflag2

//------------------------------------------------------------------
// o_phrase, o_displ: mode
// Ѱַ��ʽ����12�֣����Table 3-11 Address Generator Options for Load/Store
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
// ָ��Ĳ�����Ԫ��ana����ҪΪinsn���ø�ֵ
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

  FU_M1, FU_M2,                 // 16 x 16 bit multiply operations

  FU_D1, FU_D2,                 // 32-bit add, subtract, linear and circular address calculation
                                // Loads and stores with a 5-bit constant offset
                                // Loads and stores with 15-bit constant offset (.D2 only)
};
//------------------------------------------------------------------
// condition codes:
// ����flag��ana����ҪΪinsn���ø�ֵ�����Table 3-9 Registers That Can Be Tested by Conditional Operations
#define cond auxpref_u8[0]      // The condition code of instruction
enum cond_t : uint8_t
{
    CO_AL = 0x0,      // unconditional
    CO_B0 = 0x2,      // B0
    CO_NB0 = 0x3,     // !B0
    CO_B1 = 0x4,      // B1
    CO_NB1 = 0x5,     // !B1
    CO_B2 = 0x6,      // B2
    CO_NB2 = 0x7,     // !B2
    CO_A1 = 0x8,      // A1
    CO_NA1 = 0x9,     // !A1
    CO_A2 = 0xA,      // A2
    CO_NA2 = 0xB,     // !A2
    CO_A0 = 0xC,      // A0
    CO_NA0 = 0xD,     // !A0
    //Reserved = 1, 0xE, 0xF
};

//------------------------------------------------------------------
#define cflags auxpref_u8[1]      // Various bit definitions:
// �����flag��ana����ҪΪinsn���ø�ֵ��
#define aux_para      0x0001  // parallel execution with the next insn
#define aux_xp        0x0002  // X path is used
#define aux_src2      0x0004  // �Ƿ�Ϊ4����������ָ��
#define aux_fph       0x0008  // fetch packet header

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
struct tms66x_t : public procmod_t, public event_listener_t
{
    bool flow = false;
    ssize_t idaapi on_event(ssize_t msgid, va_list va);
};

int idaapi ana16(insn_t* insn, fetch_packet_t* fp);
int idaapi ana32(insn_t* insn, fetch_packet_t* fp);
int idaapi emu(const insn_t* insn);
int idaapi out(outctx_t* ctx);
void idaapi header(outctx_t* ctx);
void idaapi out_insn(outctx_t* ctx);
void idaapi footer(outctx_t* ctx);

#endif // _TMS66x_HPP
