/*
TODO:
1. Allow user to pass in output file name optionally
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include<sys/stat.h>    //for struct stat, stat()


typedef enum opcode {
    MOV  = 0x0,
    ADD  = 0x1,
    SUB  = 0x2,
    AND  = 0x3,
    OR   = 0x4,
    XOR  = 0x5,
    SLL  = 0x6,
    SRL  = 0x7,
    SRA  = 0x8,
    ADDI = 0x9,
    ANDI = 0xa,
    ORI  = 0xb,
    XORI = 0xc,
    SLLI = 0xd,
    SRLI = 0xe,
    SRAI = 0xf
} opcode_t;

void process_instr(FILE * output_fp, char *line, int16_t *reg_file) {
    int16_t instr, opcode, dest_reg, src1_reg, src2_reg, src2_imm;
	int16_t dest_value;

    instr = strtol(line, NULL, 16); // convert char * to int16_t

    /* Decode instr */
    opcode = (instr >> 12) & 0xF;
    dest_reg = (instr >> 8) & 0xF;
    src1_reg = (instr >> 4) & 0xF;
    src2_reg = instr & 0xF;
    if (src2_reg & 0x8) src2_imm = src2_reg | 0xFFF0; // sign-extend src2_imm
    else src2_imm = src2_reg;
    
    /*printf("instr:%hx, opcode:%hx, dest_reg:%hx, src1_reg:%hx, src2_reg:%hx, src2_imm:%hx\n", 
    instr, opcode, dest_reg, src1_reg, src2_reg, src2_imm);*/

    /* Perform computation */
    switch(opcode) {
        case MOV:  {
            dest_value = reg_file[src1_reg];
            break;
        }

        case ADD:  {
            dest_value = reg_file[src1_reg] + reg_file[src2_reg];
            break;
        }

        case SUB:  {
            dest_value = reg_file[src1_reg] - reg_file[src2_reg];
            break;
        }

        case AND:  {
            dest_value = reg_file[src1_reg] & reg_file[src2_reg];
            break;
        }

        case OR:   {
            dest_value = reg_file[src1_reg] | reg_file[src2_reg];
            break;
        }

        case XOR:  {
            dest_value = reg_file[src1_reg] ^ reg_file[src2_reg];
            break;
        }

        case SLL:  {
            dest_value = reg_file[src1_reg] << (reg_file[src2_reg] & 0xF); // can only shift by 0 ~ 15
            break;
        }

        case SRL:  {
            // cast to unsigned for logical shift
            dest_value = (uint16_t)reg_file[src1_reg] >> (reg_file[src2_reg] & 0xF); // can only shift by 0 ~ 15
            break;
        }

        case SRA:  {
            dest_value = reg_file[src1_reg] >> (reg_file[src2_reg] & 0xF); // can only shift by 0 ~ 15
            break;
        }

        case ADDI: {
            dest_value = reg_file[src1_reg] + src2_imm;
            break;
        }

        case ANDI: {
            dest_value = reg_file[src1_reg] & src2_imm;
            break;
        }

        case ORI:  {
            dest_value = reg_file[src1_reg] | src2_imm;
            break;
        }

        case XORI: {
            dest_value = reg_file[src1_reg] ^ src2_imm;
            break;
        }

        case SLLI: {
            dest_value = reg_file[src1_reg] << (src2_imm & 0xF); // can only shift by 0 ~ 15
            break;
        }

        case SRLI: {
            // cast to unsigned for logical shift
            dest_value = (uint16_t) reg_file[src1_reg] >> (src2_imm & 0xF); // can only shift by 0 ~ 15
            break;
        }

        case SRAI: {
            dest_value = reg_file[src1_reg] >> (src2_imm & 0xF); // can only shift by 0 ~ 15
            break;
        }

        default:   {
            fprintf(stderr, "Encountered unknown opcode %hx", opcode);
            exit(-1);
        }
    }

	if (dest_reg != 0) reg_file[dest_reg] = dest_value; // r0 = 0 does not change

    /* Write information to output file */
	// Instruction
	fprintf(output_fp, "%04hx", instr);
	// Dest register and new value
	fprintf(output_fp, "%x", dest_reg);
	if (dest_reg != 0) fprintf(output_fp, "%04hx", dest_value);
	else fprintf(output_fp, "%04hx", 0);
    fprintf(output_fp, "\n");
}

int main(int argc, char *argv[]) {
    /* Check test case file argument is supplied */
    if (argc < 2) {
        fprintf(stderr, "Usage: ./golden_model <test case file>\n");
        exit(-1);
    }

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_REALTIME, &start_time); // start time for benchmarking

    /* Open test case file and output file */  
    FILE *test_fp = fopen(argv[1], "r");
    if (test_fp == NULL) {
        fprintf(stderr, "Error: Failed to open file %s\n", argv[1]);
        exit(-1);
    }
    struct stat st;
    if (stat("outputs", &st) == -1)
        mkdir("outputs", 0700);
        
    FILE *output_fp = fopen("outputs/golden_output.txt", "w");
    if (output_fp == NULL) {
        fprintf(stderr, "Error: Failed to open file outputs/golden_output.txt\n");
        exit(-1);
    }

    int16_t reg_file[16] = {0}; // init register file

    /* Process test case file line by line */
    ssize_t read;
    char *line = NULL;
    size_t line_size;
    while ((read = getline(&line, &line_size, test_fp)) != -1) { // stop at end-of-file
        if (read != 5) { // 4 bytes for instr + 1 byte for new line char
            fprintf(stderr, "Error: Encountered %zu-byte line when each line should be 5 bytes\n", read);
            exit(-1);
        }
        process_instr(output_fp, line, reg_file);
    }

    /* Clean up */
    free(line);
    fclose(test_fp);
    fclose(output_fp);

    clock_gettime(CLOCK_REALTIME, &end_time); // end time for benchmarking
    int runtime = (end_time.tv_sec - start_time.tv_sec) * 1000000000 + (end_time.tv_nsec - start_time.tv_nsec);
    
    printf("Golden model finished. Output in golden_output.txt\n");
    printf("Ran for %d ms\n", runtime/1000000);
    
    return 0;
}
