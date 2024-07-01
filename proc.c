/***************************************************************/
/*                                                             */
/*   MIPS-32 Instruction Level Simulator                       */
/*                                                             */
/*   SCE212 Ajou University                                    */
/*   proc.c                                                    */
/*                                                             */
/***************************************************************/

#include <stdio.h>
#include <malloc.h>

#include "proc.h"
#include "mem.h"
#include "util.h"

/***************************************************************/
/* System (CPU and Memory) info.                                             */
/***************************************************************/
struct MIPS32_proc_t g_processor;


/***************************************************************/
/* Fetch an instruction indicated by PC                        */
/***************************************************************/
int fetch(uint32_t pc)
{
    return mem_read_32(pc);
}

/***************************************************************/
/* TODO: Decode the given encoded 32bit data (word)            */
/***************************************************************/
struct inst_t decode(int word)
{
    struct inst_t inst;
    char** tokens;
    char *line=dec_to_bin(word);
    char *temp=dec_to_bin(word);
    temp[6]='\0';
    if(word!=0){
        OPCODE(inst)=str_to_int(temp);
        if(inst.opcode==0){
            // R-type
            sprintf(temp,"%.5s",line+6);
            RS(inst)=str_to_int(temp);
            
            sprintf(temp,"%.5s",line+11);
            RT(inst)=str_to_int(temp);
            
            sprintf(temp,"%.5s",line+16);
            RD(inst)=str_to_int(temp);

            sprintf(temp,"%.5s",line+21);
            SHAMT(inst)=str_to_int(temp);

            sprintf(temp,"%.6s",line+26);
            FUNC(inst)=str_to_int(temp);
        }
        else if(inst.opcode==2 || inst.opcode==3){
            // J-type
            sprintf(temp,"%s",line+6);
            TARGET(inst)=str_to_int(temp);
        }
        else{
            // I-type
            sprintf(temp,"%.5s",line+6);
            RS(inst)=str_to_int(temp);
            
            sprintf(temp,"%.5s",line+11);
            RT(inst)=str_to_int(temp);
    
            sprintf(temp,"%s",line+16);
            IMM(inst)=str_to_int(temp);
        }
    }
    return inst;
}

/***************************************************************/
/* TODO: Execute the decoded instruction                       */
/***************************************************************/
void execute(struct inst_t inst)
{
    short op=OPCODE(inst);
    short fc,imm;
    uint32_t rs,rt,rd,shamt;
    if(op==0){
        // R
        rs=g_processor.regs[RS(inst)];
        rt=g_processor.regs[RT(inst)];
        shamt=SHAMT(inst);
        fc=FUNC(inst);
        if(fc==0x20 || fc==0x21){
            g_processor.regs[RD(inst)]=rs+rt;
        }
        else if(fc==0x22||fc==0x23){
            g_processor.regs[RD(inst)]=rs-rt;
        }
        else if(fc==0x24){
            g_processor.regs[RD(inst)]=rs&rt;
        }
        else if(fc==0x8){
            JUMP_INST(g_processor.regs[RS(inst)]);   
        }
        else if(fc==0x27){
            g_processor.regs[RD(inst)]=~(rs|rt);
        }
        else if(fc==0x25){
            g_processor.regs[RD(inst)]=rs|rt;
        }
        else if(fc==0x2b){
            g_processor.regs[RD(inst)]=(rs<rt)?1:0;
        }
        else if(fc==0x00){
            g_processor.regs[RD(inst)]=rt<<shamt;
        }
        else if(fc==0x02){
            g_processor.regs[RD(inst)]=rt>>shamt;
        }
    }
    else if(op==2||op==3){
        // J
        if(op==3){
            g_processor.regs[31]=g_processor.pc+4;
        }
        JUMP_INST(4*TARGET(inst));
    }
    else{
        // I
        rs=g_processor.regs[RS(inst)];
        imm=IMM(inst);
        if(op==0x9){
            g_processor.regs[RT(inst)]=rs+SIGN_EX(imm);
        } 
        else if(op==0xc){
            g_processor.regs[RT(inst)]=(uint32_t)rs&(uint32_t)imm;
        }
        else if(op==0x4){
            BRANCH_INST(rs==g_processor.regs[RT(inst)],g_processor.pc+(4*IMM(inst)),NULL);
        }
        else if(op==0x5){
            BRANCH_INST(rs!=g_processor.regs[RT(inst)],g_processor.pc+(4*IMM(inst)),NULL);
        }
        else if(op==0xf){
            g_processor.regs[RT(inst)]=imm<<16;
        }
        else if(op==0x23){
            g_processor.regs[RT(inst)]=mem_read_32(g_processor.regs[RS(inst)]+imm);
        }
        else if(op==0xd){
            g_processor.regs[RT(inst)]=(uint32_t)rs|(uint32_t)imm;
        }
        else if(op==0xb){   
            g_processor.regs[RT(inst)]=((uint32_t)rs<(uint32_t)imm)?1:0;
        }
        else if(op==0x2b){
            mem_write_32(mem_read_32(g_processor.regs[RS(inst)]+imm),g_processor.regs[RT(inst)]);
        }
    }
    
    if((MEM_TEXT_START+g_processor.input_insts*4)==g_processor.pc){
        g_processor.running=FALSE;
    }
}

/***************************************************************/
/* Advance a cycle                                             */
/***************************************************************/
void cycle()
{
    int inst_reg;
    struct inst_t inst;

    // 1. fetch
    inst_reg = fetch(g_processor.pc);
    g_processor.pc += BYTES_PER_WORD;

    // 2. decode
    inst = decode(inst_reg);

    // 3. execute
    execute(inst);

    // 4. update stats
    g_processor.num_insts++;
    g_processor.ticks++;
}

/***************************************************************/
/*                                                             */
/* Procedure : rdump                                           */
/*                                                             */
/* Purpose   : Dump current register and bus values to the     */
/*             output file.                                    */
/*                                                             */
/***************************************************************/
void rdump() {
    int k;

    printf("\n[INFO] Current register values :\n");
    printf("-------------------------------------\n");
    printf("PC: 0x%08x\n", g_processor.pc);
    printf("Registers:\n");
    for (k = 0; k < MIPS_REGS; k++)
        printf("R%d: 0x%08x\n", k, g_processor.regs[k]);
}



/***************************************************************/
/*                                                             */
/* Procedure : run n                                           */
/*                                                             */
/* Purpose   : Simulate MIPS for n cycles                      */
/*                                                             */
/***************************************************************/
void run(int num_cycles) {
    int i;

    if (g_processor.running == FALSE) {
        printf("[ERROR] Can't simulate, Simulator is halted\n\n");
        return;
    }

    printf("[INFO] Simulating for %d cycles...\n", num_cycles);
    for (i = 0; i < num_cycles; i++) {
        if (g_processor.running == FALSE) {
            printf("[INFO] Simulator halted\n");
            break;
        }
        cycle();
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : go                                              */
/*                                                             */
/* Purpose   : Simulate MIPS until HALTed                      */
/*                                                             */
/***************************************************************/
void go() {
    if (g_processor.running == FALSE) {
        printf("[ERROR] Can't simulate, Simulator is halted\n\n");
        return;
    }

    printf("[INFO] Simulating...\n");
    while (g_processor.running)
        cycle();
    printf("[INFO] Simulator halted\n");
}
