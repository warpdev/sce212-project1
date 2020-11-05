#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>

/*
 * For debug option. If you want to debug, set 1.
 * If not, set 0.
 */
#define DEBUG 1

#define MAX_SYMBOL_TABLE_SIZE   1024
#define MEM_TEXT_START          0x00400000
#define MEM_DATA_START          0x10000000
#define BYTES_PER_WORD          4
#define INST_LIST_LEN           20

/******************************************************
 * Structure Declaration
 *******************************************************/

typedef struct inst_struct {
    char *name;
    char *op;
    char type;
    char *funct;
} inst_t;

typedef struct symbol_struct {
    char name[32];
    uint32_t address;
} symbol_t;

enum section { 
    DATA = 0,
    TEXT,
    MAX_SIZE
};

/******************************************************
 * Global Variable Declaration
 *******************************************************/

inst_t inst_list[INST_LIST_LEN] = {       //  idx
    {"addiu",   "001001", 'I', ""},       //    0
    {"addu",    "000000", 'R', "100001"}, //    1
    {"and",     "000000", 'R', "100100"}, //    2
    {"andi",    "001100", 'I', ""},       //    3
    {"beq",     "000100", 'I', ""},       //    4
    {"bne",     "000101", 'I', ""},       //    5
    {"j",       "000010", 'J', ""},       //    6
    {"jal",     "000011", 'J', ""},       //    7
    {"jr",      "000000", 'R', "001000"}, //    8
    {"lui",     "001111", 'I', ""},       //    9
    {"lw",      "100011", 'I', ""},       //   10
    {"nor",     "000000", 'R', "100111"}, //   11
    {"or",      "000000", 'R', "100101"}, //   12
    {"ori",     "001101", 'I', ""},       //   13
    {"sltiu",   "001011", 'I', ""},       //   14
    {"sltu",    "000000", 'R', "101011"}, //   15
    {"sll",     "000000", 'R', "000000"}, //   16
    {"srl",     "000000", 'R', "000010"}, //   17
    {"sw",      "101011", 'I', ""},       //   18
    {"subu",    "000000", 'R', "100011"}  //   19
};

symbol_t SYMBOL_TABLE[MAX_SYMBOL_TABLE_SIZE]; // Global Symbol Table

uint32_t symbol_table_cur_index = 0; // For indexing of symbol table

/* Temporary file stream pointers */
FILE *data_seg;
FILE *text_seg;

/* Size of each section */
uint32_t data_section_size = 0;
uint32_t text_section_size = 0;

/******************************************************
 * Function Declaration
 *******************************************************/

/* Change file extension from ".s" to ".o" */
char* change_file_ext(char *str) {
    char *dot = strrchr(str, '.');

    if (!dot || dot == str || (strcmp(dot, ".s") != 0))
        return NULL;

    str[strlen(str) - 1] = 'o';
    return "";
}

/* Add symbol to global symbol table */
void symbol_table_add_entry(symbol_t symbol)
{
    SYMBOL_TABLE[symbol_table_cur_index++] = symbol;
#if DEBUG
    printf("%s: 0x%08x\n", symbol.name, symbol.address);
#endif
}

/* Convert integer number to binary string */
char* num_to_bits(unsigned int num, int len) 
{
    char* bits = (char *) malloc(len+1);
    int idx = len-1, i;
    while (num > 0 && idx >= 0) {
        if (num % 2 == 1) {
            bits[idx--] = '1';
        } else {
            bits[idx--] = '0';
        }
        num /= 2;
    }
    for (i = idx; i >= 0; i--){
        bits[i] = '0';
    }
    bits[len] = '\0';
    return bits;
}

/* Record .text section to output file */
void record_text_section(FILE *output) 
{
    uint32_t cur_addr = MEM_TEXT_START;
    char line[1024];

    /* Point to text_seg stream */
    rewind(text_seg);

    /* Print .text section */
    while (fgets(line, 1024, text_seg) != NULL) {
        char inst[0x1000] = {0};
        char op[32] = {0};
        char label[32] = {0};
        char type = '0';
        int i, idx = 0;
        int rs, rt, rd, shamt;
        int addr;
        uint32_t imm;

        rs = rt = rd = imm = shamt = addr = 0;
#if DEBUG
        printf("0x%08x: ", cur_addr);
        printf("%s",line);
#endif
        /* Find the instruction type that matches the line */
        /* blank */
        char *temp;
        temp=strtok(line,"\t\n");
        for(i=0; i<INST_LIST_LEN; i++)
        {
            if(!strcmp(inst_list[i].name,temp))
            {
                strcpy(inst,inst_list[i].name);
                strcpy(op,inst_list[i].op);
                type=inst_list[i].type;
                idx=i;
                break;
            }
        }

        switch (type) {
            case 'R':
                /* blank */
                fprintf(output,"%s",op);
                if(idx==8)
                {
                    temp=strtok(NULL,", \t\n");
                    rs=atoi(&temp[1]);
                }
                else
                {
                    temp=strtok(NULL,", \n\t");
                    rd=atoi(&temp[1]);
                    if(idx!=16 && idx!=17)
                    {
                        temp=strtok(NULL,", \n\t");
                        rs=atoi(&temp[1]);
                    }
                    temp=strtok(NULL,", \n\t");
                    rt=atoi(&temp[1]);
                    if(idx==16 || idx==17)
                    {
                        temp=strtok(NULL,", \t\n");
                        shamt=atoi(temp);
                    }
                }
                
                fprintf(output,"%s",num_to_bits(rs,5));
                fprintf(output,"%s",num_to_bits(rt,5));
                fprintf(output,"%s",num_to_bits(rd,5));
                fprintf(output,"%s",num_to_bits(shamt,5));
                if(atoi(inst_list[idx].funct))
                {
                    fprintf(output,"%s",inst_list[idx].funct);
                }
                else
                {
                    fprintf(output,"%s",num_to_bits(0,6));
                }
                
#if DEBUG
                printf("op:%s rs:$%d rt:$%d rd:$%d shamt:%d funct:%s\n",
                        op, rs, rt, rd, shamt, inst_list[idx].funct);
#endif
                break;

            case 'I':
                /* blank */
                fprintf(output,"%s",op);
                if(idx==4 || idx==5)
                {
                    temp=strtok(NULL,", ()\n\t");
                    rs=atoi(&temp[1]);
                    temp=strtok(NULL,", ()\t\n");
                    rt=atoi(&temp[1]);
                    temp=strtok(NULL,", ()\t\n");
                    int tmp=0;
                    for(i=0; i<symbol_table_cur_index; i++)
                    {
                        if(!strcmp(temp,SYMBOL_TABLE[i].name))
                        {
                            tmp=SYMBOL_TABLE[i].address;
                            break;
                        }
                    }
                    printf("0x%08x 0x%08x\n",tmp,cur_addr);
                    imm=(tmp-cur_addr-1)/4;
                }
                else
                {
                    temp=strtok(NULL,", ()\n\t");
                    rt=atoi(&temp[1]);
                    temp=strtok(NULL,", ()\t\n");
                    if(idx==10 || idx==18)
                    {
                        if(strchr(temp,'x'))
                        {
                            imm=strtol(temp,NULL,16);
                        }
                        else
                        {
                            imm=atoi(temp);
                        }
                        temp=strtok(NULL,", ()\t\n");
                        rs=atoi(&temp[1]);
                    }
                    else
                    {
                        if(idx!=9)
                        {
                            rs=atoi(&temp[1]);
                            temp=strtok(NULL,", ()\t\n");
                        }
                        if(strchr(temp,'x'))
                        {
                            imm=strtol(temp,NULL,16);
                        }
                        else
                        {
                            imm=atoi(temp);
                        }
                    }
                    
                }
                
                
                fprintf(output,"%s",num_to_bits(rs,5));
                fprintf(output,"%s",num_to_bits(rt,5));
                fprintf(output,"%s",num_to_bits(imm,16));
#if DEBUG
                printf("op:%s rs:$%d rt:$%d imm:0x%x\n",
                        op, rs, rt, imm);
#endif
                break;

            case 'J':
                /* blank */
                fprintf(output,"%s",op);
                temp=strtok(NULL,", \t\n");
                for(i=0; i<symbol_table_cur_index; i++)
                {
                    if(!strcmp(temp,SYMBOL_TABLE[i].name))
                    {
                        addr=SYMBOL_TABLE[i].address;
                        break;
                    }
                }
                addr=(addr<<4);
                addr=(addr>>6);
                fprintf(output,"%s",num_to_bits(addr,26));
                
                
                
#if DEBUG
                printf("op:%s addr:%i\n", op, addr);
#endif
                break;

            default:
                break;
        }
        fprintf(output, "\n");

        cur_addr += BYTES_PER_WORD;
    }
}

/* Record .data section to output file */
void record_data_section(FILE *output)
{
    uint32_t cur_addr = MEM_DATA_START;
    char line[1024];

    /* Point to data segment stream */
    rewind(data_seg);

    /* Print .data section */
    while (fgets(line, 1024, data_seg) != NULL) {
        /* blank */
        char *temp;
        uint32_t num;
        char _line[1024] = {0};
        strcpy(_line,line);
        temp=strtok(_line,"\t\n");
        temp=strtok(NULL,"\t\n");
        if(strchr(temp,'x'))
        {
            num=strtol(temp,NULL,16);
        }
        else
        {
            num=atoi(temp);
        }
        fprintf(output,"%s\n",num_to_bits(num,32));
        
        
#if DEBUG
        printf("0x%08x: ", cur_addr);
        printf("%s", line);
#endif
        cur_addr += BYTES_PER_WORD;
    }
}

/* Fill the blanks */
void make_binary_file(FILE *output)
{
#if DEBUG
    char line[1024] = {0};
    rewind(text_seg);
    /* Print line of text segment */
    while (fgets(line, 1024, data_seg) != NULL) {
        printf("%s",line);
    }
    printf("text section size: %d, data section size: %d\n",
            text_section_size, data_section_size);
#endif

    /* Print text section size and data section size */
    fprintf(output,"%s\n",num_to_bits(text_section_size,32));
    fprintf(output,"%s\n",num_to_bits(data_section_size,32));
    /* blank *////

    /* Print .text section */
    record_text_section(output);

    /* Print .data section */
    record_data_section(output);
}

/* Fill the blanks */
void make_symbol_table(FILE *input)
{
    char line[1024] = {0};
    uint32_t address = 0;
    enum section cur_section = MAX_SIZE;

    /* Read each section and put the stream */
    while (fgets(line, 1024, input) != NULL) {
        char *temp;
        char _line[1024] = {0};
        strcpy(_line, line);
        temp = strtok(_line, "\t\n");

        /* Check section type */
        if (!strcmp(temp, ".data")) {
            /* blank */
            cur_section=DATA;
            address=0;
            data_seg = tmpfile();
            continue;
        }
        else if (!strcmp(temp, ".text")) {
            /* blank */
            cur_section=TEXT;
            address=0;
            text_seg = tmpfile();
            continue;
        }

        /* Put the line into each segment stream */
        if (cur_section == DATA) {
            /* blank */
            if(strchr(temp,':'))
            {
                symbol_t tsym;
                strncpy(tsym.name,temp,strlen(temp)-1);
                tsym.address=MEM_DATA_START+data_section_size;
                symbol_table_add_entry(tsym);
            }
            data_section_size+=BYTES_PER_WORD;
            while(temp)
            {
                if(!strchr(temp,':'))
                {
                    fprintf(data_seg,"%s\t",temp);
                }
                temp=strtok(NULL,"\t\n");
            }
            fprintf(data_seg,"\n");
        }
        else if (cur_section == TEXT) {
            /* blank */
            if(strchr(temp,':'))
            {
                symbol_t tsym;
                strcpy(tsym.name,strtok(temp,":"));
                tsym.address=MEM_TEXT_START+text_section_size;
                symbol_table_add_entry(tsym);
                continue;
            }
            text_section_size+=BYTES_PER_WORD;
            
            while(temp)
            {
                if(!strcmp(temp,"la"))
                {
                    int trd=0;
                    fprintf(text_seg,"%s\t","lui");
                    temp=strtok(NULL,", \n");
                    trd=atoi(&temp[1]);
                    fprintf(text_seg,"%s, ",temp);
                    temp=strtok(NULL,", \n");
                    int i;
                    uint32_t tmp=0;
                    for(i=0; i<symbol_table_cur_index; i++)
                    {
                        if(!strcmp(temp,SYMBOL_TABLE[i].name))
                        {
                            tmp=SYMBOL_TABLE[i].address;
                            break;
                        }
                    }
                    fprintf(text_seg,"%d",(tmp>>16));
                    if((tmp<<16))
                    {
                        text_section_size+=BYTES_PER_WORD;
                        tmp=tmp<<16;
                        tmp=tmp>>16;
                        fprintf(text_seg,"\n%s\t","ori");
                        fprintf(text_seg,"$%d, $%d, ",trd,trd);
                        fprintf(text_seg,"%d",tmp);
                    }
                }
                else
                {
                    fprintf(text_seg,"%s\t",temp);
                }
                temp=strtok(NULL,"\t\n");
            }
            fprintf(text_seg,"\n");
        }

        address += BYTES_PER_WORD;
    }
    
}

/******************************************************
 * Function: main
 *
 * Parameters:
 *  int
 *      argc: the number of argument
 *  char*
 *      argv[]: array of a sting argument
 *
 * Return:
 *  return success exit value
 *
 * Info:
 *  The typical main function in C language.
 *  It reads system arguments from terminal (or commands)
 *  and parse an assembly file(*.s).
 *  Then, it converts a certain instruction into
 *  object code which is basically binary code.
 *
 *******************************************************/

int main(int argc, char* argv[])
{
    FILE *input, *output;
    char *filename;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <*.s>\n", argv[0]);
        fprintf(stderr, "Example: %s sample_input/example?.s\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Read the input file */
    input = fopen(argv[1], "r");
    if (input == NULL) {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }

    /* Create the output file (*.o) */
    filename = strdup(argv[1]); // strdup() is not a standard C library but fairy used a lot.
    if(change_file_ext(filename) == NULL) {
        fprintf(stderr, "'%s' file is not an assembly file.\n", filename);
        exit(EXIT_FAILURE);
    }

    output = fopen(filename, "w");
    if (output == NULL) {
        perror("ERROR");
        exit(EXIT_FAILURE);
    }

    /******************************************************
     *  Let's complete the below functions!
     *
     *  make_symbol_table(FILE *input)
     *  make_binary_file(FILE *output)
     *  ├── record_text_section(FILE *output)
     *  └── record_data_section(FILE *output)
     *
     *******************************************************/
    make_symbol_table(input);
    make_binary_file(output);

    fclose(input);
    fclose(output);

    return 0;
}
