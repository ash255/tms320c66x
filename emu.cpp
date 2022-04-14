#include "fetch_packet.h"
#include "tms66x.h"

/* 
    ������������
    ������ʾ��Ӧ�����һ��0x8F3CE4����������
00036E70   021E722A           MVK.S2        0x3ce4,B4
00036E74   020047EA           MVKH.S2       0x8f0000,B4 ;B4=0x8f3ce4
*/
static void data_quote(const insn_t* insn, const op_t* x)
{

}

//�ڷ�����֧�ĵ�ַ����delay slot��Ѱ���Ƿ���addkpc xx, B3, x����
static bool check_func_1(const insn_t* insn)
{
    insn_t next_ins;
    ea_t next_adr = insn->ea + insn->size;
    int loop = 6;
    if (insn->itype == TMS6_bnop)
        loop -= insn->Op2.value;
    while (loop > 0)
    {
        if (decode_insn(&next_ins, next_adr) == 0)
            break;
        if ((next_ins.cflags & aux_para) == 0)
            loop -= 1;
        if (next_ins.itype == TMS6_nop)
            loop -= next_ins.Op1.value;
        if (next_ins.itype == TMS6_addkpc && next_ins.Op2.reg == rB3)
            return true;
        next_adr = next_ins.ea + next_ins.size;
    }
    return false;
}

//�ڷ�֧��ת���loop��ָ����Ѱ���Ƿ��ж�B15�Ĵ�����ջ�Ĳ���
static bool check_func_2(const insn_t* insn)
{
    insn_t next_ins;
    ea_t next_adr = insn->Op1.addr;
    int loop = 6;   
    while (loop)
    {
        decode_insn(&next_ins, next_adr);
        int type = next_ins.itype;
        if (type == TMS6_stdw || type == TMS6_stnw || type == TMS6_stndw
            || type == TMS6_stb || type == TMS6_stbu || type == TMS6_sth
            || type == TMS6_sthu || type == TMS6_stw)
        {
            //�Ƿ����*B15--[ucst5]ģʽ
            if (next_ins.Op2.type == o_displ && next_ins.Op2.reg == rB15 && next_ins.Op2.mode == MO_UCST_SUBSUB)
                return true;
        }
        //sub b15, 4, b15
        //add -4, b15, b15
        next_adr = next_ins.ea + next_ins.size;
        loop -= 1;
    }
    return false;
}

/*
    �����������
    callp��Ȼ�Ǻ�����ת
    b��bnop���п����Ǻ����ڷ�֧��תҲ�п����Ǻ�����ת����Ϊ������ת������ת������Ȼ��ADDKPC��������B3������ָ��
    <bnop B3,x>��<b B3>����ͨ���������ķ���ָ��
*/
static int code_quote(const insn_t* insn)
{
    if (insn->itype == TMS6_callp)
    {
        insn->add_cref(insn->Op1.addr, insn->Op1.offb, fl_CN);
        return 1;
    }

    return 0;

    if (insn->itype == TMS6_bnop || insn->itype == TMS6_b)
    {
        bool check1, check2;

        check1 = check_func_1(insn);
        check2 = check_func_2(insn);

        if (check2 && check1)
        {
            insn->add_cref(insn->Op1.addr, insn->Op1.offb, fl_CN);
            return 1;
        }
            
    }
    return 0;
}

static void handle_operand(const insn_t* insn, const op_t* x, bool isload)
{
    switch (x->type)
    {
    case o_regpair:
    case o_reg:
    case o_phrase:
    case o_spmask:
    case o_stgcyc:
    case o_displ:
        break;
    case o_imm:
        if (isload)
            data_quote(insn, x);
        break;
    case o_near:
        if(code_quote(insn) == 0)
            insn->add_cref(x->addr, x->offb, fl_JN);
        break;
    }
}

int emu(const insn_t* insn)
{
	fetch_packet_t fp;
	update_fetch_packet(insn->ea, &fp);
    uint32 Feature = insn->get_canon_feature(ph);    //get instruction's CF_XX flags

	flags_t F = get_flags(insn->ea);
	if (Feature & CF_USE1) handle_operand(insn, &insn->Op1, true);
	if (Feature & CF_USE2) handle_operand(insn, &insn->Op2, true);
	if (Feature & CF_USE3) handle_operand(insn, &insn->Op3, true);
													      
	if (Feature & CF_CHG1) handle_operand(insn, &insn->Op1, false);
	if (Feature & CF_CHG2) handle_operand(insn, &insn->Op2, false);
	if (Feature & CF_CHG3) handle_operand(insn, &insn->Op3, false);

	if ((Feature & CF_STOP) == 0)
		add_cref(insn->ea, insn->ea + insn->size, fl_F);	//����ָ������

	return 1;
}