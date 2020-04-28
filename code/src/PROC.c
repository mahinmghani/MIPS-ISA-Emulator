#include <stdio.h>	/* fprintf(), printf() */
#include <stdlib.h>	/* atoi() */
#include <stdint.h>	/* uint32_t */

#include "RegFile.h"
#include "Syscall.h"
#include "utils/heap.h"
#include "elf_reader/elf_reader.h"

// These two functions sign extend 16 and 8 bit values respectively
uint32_t sign_extend_16(uint16_t immediate);
uint32_t zero_extend_16(uint16_t immediate);
uint32_t sign_extend_8(uint8_t immediate);
uint32_t zero_extend_8(uint8_t immediate);
void program_counter_update(uint32_t* ProgramCounter, bool* branch_delay_slot, bool* take_branch,  uint32_t branch_PC);
void pc_update(uint32_t offset, uint32_t* current_PC, uint32_t* next_PC);

int main(int argc, char * argv[]) {

	/*
	 * This variable will store the maximum
	 * number of instructions to run before
	 * forcibly terminating the program. It
	 * is set via a command line argument.
	 */
	uint32_t MaxInstructions;

	/*
	 * This variable will store the address
	 * of the next instruction to be fetched
	 * from the instruction memory.
	 */
	uint32_t ProgramCounter;

	/*
	 * This variable will store the instruction
	 * once it is fetched from instruction memory.
	 */
	uint32_t CurrentInstruction;

	//IF THE USER HAS NOT SPECIFIED ENOUGH COMMAND LINE ARUGMENTS
	if(argc < 3){

		//PRINT ERROR AND TERMINATE
		fprintf(stderr, "ERROR: Input argument missing!\n");
		fprintf(stderr, "Expected: file-name, max-instructions\n");
		return -1;

	}

     	//CONVERT MAX INSTRUCTIONS FROM STRING TO INTEGER	
	MaxInstructions = atoi(argv[2]);	

	//Open file pointers & initialize Heap & Regsiters
	initHeap();
	initFDT();
	initRegFile(0);

	//LOAD ELF FILE INTO MEMORY AND STORE EXIT STATUS
	int status = LoadOSMemory(argv[1]);

	//IF LOADING FILE RETURNED NEGATIVE EXIT STATUS
	if(status < 0){ 
		
		//PRINT ERROR AND TERMINATE
		fprintf(stderr, "ERROR: Unable to open file at %s!\n", argv[1]);
		return status; 
	
	}

	printf("\n ----- BOOT Sequence ----- \n");
	printf("Initializing sp=0x%08x; gp=0x%08x; start=0x%08x\n", exec.GSP, exec.GP, exec.GPC_START);

	RegFile[28] = exec.GP;
	RegFile[29] = exec.GSP;
	RegFile[31] = exec.GPC_START;

	printRegFile();

	printf("\n ----- Execute Program ----- \n");
	printf("Max Instruction to run = %d \n",MaxInstructions);
	fflush(stdout);
	ProgramCounter = exec.GPC_START;
	
	/***************************/
	/* ADD YOUR VARIABLES HERE */
	/***************************/
  
  /*
   *  Initialize variables:
   *   opcode
   *   source register
   *   target register
   *   destination register (for R-type str)
   *   shamt (for R-type instr)
   *   funct (for R-type instr)
   *
   *   immediate (for I-type instr)
   *   label (for J-type instr)
   */

    uint8_t opcode = 0x00;                      // 6 bits
    uint8_t source_register = 0x00;             // 5 bits
    uint8_t base = 0x00;                        // 5 bits, same as source_reg but provides more clarity - use only for LOAD/STORE
    uint8_t target_register = 0x00;             // 5 bits
    uint8_t destination_register = 0x00;        // 5 bits
    uint8_t shamt = 0x00;                       // 5 bits
    uint8_t funct = 0x00;                       // 6 bits
    uint8_t loaded_byte = 0x00;                 // 8 bits, for LB
    uint8_t stored_byte = 0x00;                 // 8 bits, for SB
    uint16_t immediate = 0x0000;                // 16 bits
    uint16_t offset = 0x0000;                   // 16 bits, same as immediate but provides more clarity - use only for LOAD/STORE
    uint16_t loaded_halfword = 0x0000;          // 16 bits, for LH
    uint16_t stored_halfword = 0x0000;          // 16 bits, for SH
    uint32_t extended_offset = 0x00000000;      // 32 bits, sign extended - for LOAD/STORE
    uint32_t extended_immediate = 0x00000000;   // 32 bits, sign extended
    uint32_t pseudo_address = 0x00000000;       // 26 bits
    uint32_t loaded_word = 0x00000000;          // 32 bits
    uint32_t stored_word = 0x00000000;          // 32 bits, for SW
    uint32_t extended_loaded_byte = 0x00000000; // 32 bits, sign extended - for LB
    uint32_t extended_loaded_halfword = 0x00000000; // 32 bits, sign ext. for LH

    int instruction_counter = 0;

  // Signals the presence/absence of a load/branch delay slot
    bool load_delay_slot = false;
    bool branch_delay_slot = false;
    bool link = false;
    bool take_branch = false;
    uint8_t load_delay_target = 0x00;           // 5 bits, indicates which register cannot be read in the load delay slot
    uint32_t branch_PC = 0x00000000;           // 32 bits, to be used for branches/jumps

	uint32_t next_PC = ProgramCounter;

	int i;
	for(i = 0; i < MaxInstructions; i++) {

		//FETCH THE INSTRUCTION AT 'ProgramCounter'		
		CurrentInstruction = readWord(ProgramCounter,true);

		//PRINT CONTENTS OF THE REGISTER FILE	
		printRegFile();
		
		/********************************/
		/* ADD YOUR IMPLEMENTATION HERE */
		/********************************/
    
        opcode = (CurrentInstruction & 0xFC000000) >> 26;

        if(opcode == 0x00){
            printf("R-type instruction found\n");
            source_register = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            destination_register = (CurrentInstruction & 0x0000F800) >> 11;
            shamt = (CurrentInstruction & 0x000007C0) >> 6;
            funct = (CurrentInstruction & 0x0000003F);

            
			if(funct == 0x20){
				/*
					ADD
				*/
				printf ("ADD instruction found!\n");
				RegFile[destination_register] = (RegFile[source_register] + RegFile[target_register]);
				//ProgramCounter += 0x00000004;
			}
			else if(funct == 0x21){
				/*
					ADDU - same as ADD
				*/
                printf ("ADDU instruction found!\n");
                RegFile[destination_register] = (RegFile[source_register] + RegFile[target_register]);
			}
			else if(funct == 0x22){
				/*
					SUB
				*/
                printf ("SUB instruction found!\n");
                RegFile[destination_register] = RegFile[source_register] - RegFile[target_register];
			}
			else if(funct == 0x23){
				/*
					SUBU
				*/
                printf ("SUBU instruction found!\n");
                RegFile[destination_register] = RegFile[source_register] - RegFile[target_register];
			}
			else if(funct == 0x24){
				/*
					AND
				*/
				printf("AND instruction found!\n");
				RegFile[destination_register] = RegFile[source_register] & RegFile[target_register];
			}
			else if(funct == 0x25){
				/*
					OR
				*/
                printf ("OR instruction found!\n");
                RegFile[destination_register] = RegFile[source_register] | RegFile[target_register];
			}
			else if(funct == 0x26){
				/*
					XOR
				*/
                printf ("XOR instruction found!\n");
                RegFile[destination_register] = RegFile[source_register] ^ RegFile[target_register];
			}
			else if(funct == 0x27){
				/*
					NOR
				*/
                printf ("NOR instruction found!\n");
                RegFile[destination_register] = ~(RegFile[source_register] | RegFile[target_register]);
			}
			else if(funct == 0x2A){
				/*
					SLT: if $rs < $rt: $rd = 1	 else: $rd = 0
					Complication is that it must do a signed comparison
				*/
				printf("SLT instruction found!\n");
				int32_t rs = RegFile[source_register];
				int32_t rt = RegFile[target_register];
				if(rs < rt){
					RegFile[destination_register] = 0x00000001;
				}else{
					RegFile[destination_register] = 0x00000000;
				}
			}
			else if(funct == 0x2B){
				/*
					SLTU: Similar to SLT but no need to type cast
				*/
				printf("SLTU instruction found!\n");
                if (RegFile[source_register] < RegFile[target_register]){
                    RegFile[destination_register] = 0x00000001;
                }
                else{
                    RegFile[destination_register] = 0x00000000;
                }
			}
			else if(funct == 0x00){
				/*
					SLL
				*/
				printf("SLL instruction found!\n");
				RegFile[destination_register] = RegFile[target_register] << shamt;
			}
			else if(funct == 0x02){
				/*
					SRL
				*/
				printf("SRL instruction found!\n");
				RegFile[destination_register] = RegFile[target_register] >> shamt;
			}
			else if(funct == 0x03){
                /*
                 *  SRA - Shift Right Arithmetic
                 *  RegFile[destination_register] <- RegFile[target_register] >> shamt
                 *  Need to extract MSB (sign) of RegFile[target_register]
                 *  If MSB = 0 (positive), then its a simple right shift
                 *  If MSB = 1 (negative), then its a right shift followed by
                 *  OR-ing with an 0xFFFFFFFF mask shifted up by however many
                 *  bits we need to set to 1, which is 32 - shift amount
                 */
				printf("SRA instruction found!\n");
                if(false && (target_register == load_delay_target)){
                    /*
                     *  NOP - need to respect load delay slot
                     */
                }else{
                    /*
                     *  Load delay slot is not a problem here
                     */
                    uint8_t sign = RegFile[target_register] >> 31;
                    if(sign == 0x01){
                        RegFile[destination_register] = (RegFile[target_register] >> shamt) | ((uint32_t)0xFFFFFFFF << ((uint32_t)32 - shamt));
                    }else{
                        RegFile[destination_register] = RegFile[target_register] >> shamt;
                    }
                }
                load_delay_slot = false;
                // Vanilla PC update
                //ProgramCounter
            }
			else if(funct == 0x04){
				/*
					SLLV
				*/	
                printf ("SLLV instruction found!\n");
                RegFile[destination_register] = RegFile[target_register] << (RegFile[source_register] & 0x0000001F);
			}
			else if(funct == 0x06){
				/*
					SRLV
				*/
                printf ("SRLV instruction found!\n");
                RegFile[destination_register] = RegFile[target_register] >> (RegFile[source_register] & 0x0000001F);
			}
			else if(funct == 0x07){
                /*
                 *  SRAV - Shift Right Arithmetic Variable
                 *  RegFile[destination_register] <- RegFile[target_register] >> RegFile[source_register]
                 *  Need to extract MSB (sign) of RegFile[target_register]
                 *  if MSB = 0 (+ve), then its a simple right shift
                 *  if MSB = 1 (-ve), then its a right shift followed by OR-ing
                 *  with an 0xFFFFFFFF mask shifted up by however many bits we
                 *  need to set to 1, which is 32 - RegFile[source_register]
                 */
				printf("SRAV instruction found!\n");
                uint8_t sign = RegFile[target_register] >> 31;
                uint32_t shift = RegFile[source_register] & 0x0000001F;
                if(sign == 0x01){
                    RegFile[destination_register] = (RegFile[target_register] >> shift) | (0xFFFFFFFF << (32 - shift));
                }else{
                    RegFile[destination_register] = (RegFile[target_register] >> shift);
                } 
                load_delay_slot = false;
                // Vanilla PC update
                //ProgramCounter
            }
			else if(funct == 0x08){
                /*
                 *  JR - Jump Register
                 *  PC <- RS
                 */
				printf("JR instruction found!\n");
                printf("Source register == %X\n", source_register);
                branch_PC = RegFile[source_register];
                take_branch = true;
            }
			else if(funct == 0x09){
                /*
                 *  JALR - Jump and Link Regsiter
                 *
                 */
				printf("JALR instruction found!\n");
                // if(destination_register != 0x1F){
                //     printf("EXCEPTION: Destination register is not $ra but <%d> in JALR\n", destination_register);
                // }
                RegFile[destination_register] = ProgramCounter + 0x000000008; // $ra contains return address
                branch_PC = RegFile[source_register];
                take_branch = true;
            }
			else if(funct == 0x10){
				/*
					MFHI
				*/	
                printf ("MFHI instruction found!\n");
                RegFile[destination_register] = RegFile[32];
            }
			else if(funct == 0x11){
            	/*
					MTHI
				*/
                printf ("MTHI instruction found!\n");
                RegFile[32] = RegFile[source_register];
            }
			else if(funct == 0x12){
            	/*
					MFLO
				*/
                printf ("MFLO instruction found!\n");
                RegFile[destination_register] = RegFile[33];
            }
			else if(funct == 0x13){
            	/*
					MTLO
				*/
                printf ("MTLO instruction found!\n");
                RegFile[33] = RegFile[source_register];
            }
			else if(funct == 0x18){
            	/*
					MULT
				*/
                printf ("MULT instruction found!\n");
                int temp_sign = 1;
                int64_t signed_source_register = RegFile[source_register];
                if(RegFile[source_register] >> 31 == 0x01){
                    signed_source_register |= 0xFFFFFFFF00000000;
                }
				else{
					signed_source_register &= 0x00000000FFFFFFFF;
				}
                int64_t signed_target_register = RegFile[target_register];
                if(RegFile[target_register] >> 31 == 0x01){
                    signed_target_register |= 0xFFFFFFFF00000000;
                }
				else{
					signed_target_register &= 0x00000000FFFFFFFF;
				}
                int64_t signed_product = signed_source_register * signed_target_register;
                RegFile[33] = signed_product;       // LO
                RegFile[32] = signed_product >> 32; // HI
            }
			else if(funct == 0x19){
            	/*
					MULTU
				*/
                printf ("MULTU instruction found!\n");
                uint64_t temp_source_register = RegFile[source_register] & 0x00000000FFFFFFFF;
                uint64_t temp_target_register = RegFile[target_register] & 0x00000000FFFFFFFF;
                uint64_t temp_product = temp_source_register * temp_target_register;
				// printf("\n\n temp_source_register == %llx \t %llu \n", temp_source_register, temp_source_register);
// 				printf(" temp_target_register == %llx \t %llu \n", temp_target_register, temp_target_register);
// 				printf(" temp_product == %llx \t %llu \n", temp_product, temp_product);
// 				printf(" hi == %llx \t %llu \n", temp_product >> 32, temp_product >> 32);
// 				printf(" lo == %llx \t %llu \n\n", temp_product, temp_product);
				
				RegFile[32] = temp_product >> 32;                                   //Storing the upper 32 but in HI register
                RegFile[33] = temp_product;                                         //Storing the lower 32 bit in LO register
                
            }
			else if(funct == 0x1A){
            	/*
					DIV
				*/
                printf("DIV instruction found!\n");
                int32_t signed_source_register = RegFile[source_register];
                int32_t signed_target_register = RegFile[target_register];

                RegFile[33] = signed_source_register/signed_target_register;
                RegFile[32] = signed_source_register % signed_target_register;
            }
			else if(funct == 0x1B){
            	/*
					DIVU
				*/
                printf ("DIVU instruction found!\n");
                uint32_t temp_source_register = RegFile[source_register] ;
                uint32_t temp_target_register = RegFile[target_register] ;
                RegFile[32] = temp_source_register % temp_target_register;
                RegFile[33] = temp_source_register / temp_target_register;
            }
            else if(funct == 0x0C){
				/*
					SYSCALL: Very simple, just have to call the syscall function given to us
				*/
				SyscallExe(RegFile[2]); // Systemcall ID found in $2
				// ProgramCounter += 0x00000004;
                
            }
        }
		else if(opcode == 0x08){
            /*
            *  ADDI - Add immediate
            */

            printf("I-type instruction found: ADDI\n");
            // Instruction decode
            source_register = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            immediate = (CurrentInstruction & 0x0000FFFF);
            // Sign extension
            extended_immediate = sign_extend_16(immediate);
            printf("source_register: %X\n", source_register);
            printf("RegFile[%d] == %X\n", source_register, RegFile[source_register]);
            printf("RegFile[%d] == %X\n", target_register, RegFile[target_register]);
            printf("target_register: %X\n", target_register);
            printf("immediate: hex == %X, dec == %d\n", immediate, immediate);
            RegFile[target_register] = RegFile[source_register] + extended_immediate;
            // Vanilla PC update
            //ProgramCounter;
        }
		else if(opcode == 0x09){
			/*
				ADDIU
			*/
            printf ("ADDIU instruction found!\n");
            source_register = (CurrentInstruction & 0x03E00000) >> 21;
			target_register = (CurrentInstruction & 0x001F0000) >> 16;
			immediate = (CurrentInstruction & 0x0000FFFF);
            extended_immediate = sign_extend_16(immediate);
            RegFile[target_register] = RegFile[source_register] + extended_immediate;
		}
		else if(opcode == 0x0A){
			/*
				SLTI
			*/	
            printf ("SLTI instruction found!\n");

            source_register = (CurrentInstruction & 0x03E00000) >> 21;
			target_register = (CurrentInstruction & 0x001F0000) >> 16;
			immediate = (CurrentInstruction & 0x0000FFFF);

            extended_immediate = sign_extend_16(immediate);
			
			int32_t rs = RegFile[source_register];
			int32_t imm = extended_immediate;
            if (rs < imm){
                RegFile[target_register] = 0x00000001;
            }
            else{
                RegFile[target_register] = 0x00000000;
            }
		}
		else if(opcode == 0x0B){
			/*
				SLTIU - Same as SLTI but no signed comparison 
			*/
            printf ("SLTIU instruction found!\n");

            source_register = (CurrentInstruction & 0x03E00000) >> 21;
			target_register = (CurrentInstruction & 0x001F0000) >> 16;
			immediate = (CurrentInstruction & 0x0000FFFF);
            extended_immediate = sign_extend_16(immediate);

            if (RegFile[source_register] < extended_immediate){
                RegFile[target_register] = 0x00000001;
            }
            else{
                RegFile[target_register] = 0x00000000;
            }
		}
		else if(opcode == 0x0C){
			/*
				ANDI
			*/
            printf ("ANDI instruction found!\n");

            source_register = (CurrentInstruction & 0x03E00000) >> 21;
			target_register = (CurrentInstruction & 0x001F0000) >> 16;
			immediate = (CurrentInstruction & 0x0000FFFF);

            extended_immediate = zero_extend_16(immediate);

            RegFile[target_register] = (RegFile[source_register] & extended_immediate);
		}
		else if(opcode == 0x0D){
			/*
				ORI
			*/
            printf ("ORI instruction found!\n");

            source_register = (CurrentInstruction & 0x03E00000) >> 21;
			target_register = (CurrentInstruction & 0x001F0000) >> 16;
			immediate = (CurrentInstruction & 0x0000FFFF);

            extended_immediate = zero_extend_16(immediate);

            RegFile[target_register] = (RegFile[source_register] | extended_immediate);
		}
		else if(opcode == 0x0E){
			/*
				XORI
			*/
            printf ("XORI instruction found!\n");

            source_register = (CurrentInstruction & 0x03E00000) >> 21;
			target_register = (CurrentInstruction & 0x001F0000) >> 16;
			immediate = (CurrentInstruction & 0x0000FFFF);

            extended_immediate = zero_extend_16(immediate);

            RegFile[target_register] = (RegFile[source_register] ^ extended_immediate);
		}
		else if(opcode == 0x0F){
			/*
				LUI
			*/
            printf ("LUI instruction found!\n");

            source_register = (CurrentInstruction & 0x03E00000) >> 21;
			target_register = (CurrentInstruction & 0x001F0000) >> 16;
			immediate = (CurrentInstruction & 0x0000FFFF);
			extended_immediate = zero_extend_16(immediate) << 16;
            uint32_t shifted_immediate = extended_immediate;
            RegFile[target_register] = shifted_immediate;             
		}
		else if(opcode == 0x20){
            /*
            *  LB - Load Byte
            *  target register <- memory[$base + offset]
            *  instruction form: OP, base, target register, offset
            *                    6,  5   , 5              , 16
            *
            */
			printf("LB instruction found!\n");
            base = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset);  // sign extension
            /*
            *  $base = RegFile[base]
            *  loaded_byte = Memory[$base + extended_offset]
            *  'true' flag in readByte prints debug msg
            *  loaded_byte is then sign extended
            *  sign extended loaded byte is placed in RegFile[target_register]
            */
            loaded_byte = readByte(RegFile[base] + extended_offset, true);
            extended_loaded_byte = sign_extend_8(loaded_byte);
            RegFile[target_register] = extended_loaded_byte;
            load_delay_slot = true; // Now there is a load delay slot
            load_delay_target = target_register;  // Cannot use target_register content in load delay slot
            // Vanilla PC update
            //ProgramCounter;
        }
		else if(opcode == 0x21){
            /*
            *  LH - Load Halfword
            *  target_register <- memory[$base + offset]
            *  instruction form: OP, base, target register, offset
            *                    6,  5   , 5              , 16
            *
            */
			printf("LH instruction found!\n");
            base = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset);  // sign extension
            /*
            *  $base = RegFile[base]
            *  loaded_halfword = Memory[$base + extended_offset]
            *  'true' flag in readByte prints debug msg
            *  loaded_halfword << 8, populates top 8 bits in halfword
            *  loaded_halfword = loaded_halfword | Memory[$base + extended_offset + 1]
            *  'true' flag in readByte prints debug msg
            *  sign extended loaded halfword is placed in RegFile[target_register]
            */
            loaded_halfword = readByte(RegFile[base] + extended_offset, true);
            loaded_halfword = loaded_halfword << 8;
            uint16_t tmp = readByte(RegFile[base] + extended_offset + 1, true);
            loaded_halfword |= tmp;
            extended_loaded_halfword = sign_extend_16(loaded_halfword);
            RegFile[target_register] = extended_loaded_halfword;
            load_delay_slot = true;
            load_delay_target = target_register;
            // Vanilla PC update
            //ProgramCounter;
        }
		else if(opcode == 0x22){
            /*
            *  LWL - Load Word Left
            *  target_register <- target_register MERGE memory[$base + offset]
            *  form: OP, base, target register, offset
            *        6,  5   , 5              , 16
            *
            */
			printf("LWL instruction found!\n");
            base = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset);  // sign extension
            loaded_word = readWord(RegFile[base] + extended_offset,true);
            loaded_word = loaded_word & 0xFFFF0000;
            RegFile[target_register] = RegFile[target_register] & 0x0000FFFF;
            RegFile[target_register] = RegFile[target_register] | loaded_word;
        }
		else if(opcode == 0x23){
            /*
            *  LW - Load Word
            *  target_register <- Memory[$base + offset]
            */
			printf("LW instruction found!\n");
            base = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset);  // sign extension
            /*
            *  $base = RegFile[base]
            *  Address to load from is RegFile[base] + extended_offset
            *  loaded_word is then put in RegFile[target_register]
            *  load delay slot needs to be enabled
            */
            loaded_word = readWord(RegFile[base] + extended_offset, true);
            RegFile[target_register] = loaded_word;
            load_delay_slot = true;
            load_delay_target = target_register;
            // Vanilla PC update
            //ProgramCounter;
        }
		else if(opcode == 0x24){
            /*
             *  LBU - Load Byte Unsigned
             *  target_register <- Memory[$base + offset]
             */
			printf("LBU instruction found!\n");
            base = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset); 
            /*
             *  $base = RegFile[base]
             *  Address to load from is RegFile[base] + extended_offset
             *  loaded_byte is then ZERO-extended and placed in
             *  RegFile[target_register]
             */
            loaded_byte = readByte(RegFile[base] + extended_offset, true);
            extended_loaded_byte = zero_extend_8(loaded_byte);
            RegFile[target_register] = extended_loaded_byte;
            load_delay_slot = true; // Now there is a load delay slot
            load_delay_target = target_register;  // Cannot use target_register content in load delay slot
            // Vanilla PC update
            //ProgramCounter;
        }
		else if(opcode == 0x25){
            /*
            *  LHU - Load Halfword Unsigned
            *  target_register <- memory[$base + offset]
            *  instruction form: OP, base, target register, offset
            *                    6,  5   , 5              , 16
            *
            */
			printf("LHU instruction found!\n");
            base = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset);  // sign extension
            /*
            *  $base = RegFile[base]
            *  loaded_halfword = Memory[$base + extended_offset]
            *  'true' flag in readByte prints debug msg
            *  loaded_halfword << 8, populates top 8 bits in halfword
            *  loaded_halfword = loaded_halfword | Memory[$base + extended_offset + 1]
            *  'true' flag in readByte prints debug msg
            *  ZEOR extended loaded halfword is placed in RegFile[target_register]
            */
            loaded_halfword = readByte(RegFile[base] + extended_offset, true);
            loaded_halfword = loaded_halfword << 8;
            loaded_halfword |= readByte(RegFile[base] + extended_offset + 0x01, true);
            extended_loaded_halfword = zero_extend_16(loaded_halfword);
            RegFile[target_register] = extended_loaded_halfword;
            load_delay_slot = true;
            load_delay_target = target_register;
            // Vanilla PC update
            //ProgramCounter;
        }
		else if(opcode == 0x26){
            /*
             *  LWR - Load Word Right
             *  target_register <- target_register MERGE memory[$base + offset]
             *
             */
			printf("LWR instruction found!\n");
            base = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset);  // sign extension
            loaded_word = readWord(RegFile[base] + extended_offset, true);
            loaded_word = loaded_word & 0x0000FFFF;
            RegFile[target_register] = RegFile[target_register] & 0xFFFF0000;
            RegFile[target_register] = RegFile[target_register] | loaded_word;
        }
		else if(opcode == 0x28){
            /*
             *  SB - Store Byte
             *  memory[$base+offset] <- RegFile[target_register]
             *
             */
			printf("SB instruction found!\n");
            base = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset);  // sign extension
            /*
             *  $base = RegFile[base]
             *  The least-significant 8 bits of RegFile[target_register] is
             *  stored in Memory[$base + offset]
             *  
             *  Use writeByte(address, data, debug) func for this.
             *  debug == true - to get a debug msg from writeByte
             *
             *  Need to check if instr. is in a load delay slot and if we
             *  are reading from a load delay target
             */
            stored_byte = RegFile[target_register]; // Automatically takes least significant byte of RegFile[target_Register]
            /*
             * Passing in the address, the data, and the debug flag
             */
            writeByte(RegFile[base] + extended_offset, stored_byte, true);

        }
		else if(opcode == 0x29){
            /*
             *  SH - Store Halfword
             *  memory[$base + offset] <- RegFile[target_register]
             */
			printf("SH instruction found!\n");
            base = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset);  // sign extension
            /*
             *  $base = RegFile[base]
             *  The least-significant 16 bits of RegFile[target_register] is
             *  stored in Memory[$base + offset]
             *  
             *  Use writeByte(address, data, debug) TWICE to do this.
             *  debug == true - to get a debug msg from writeByte
             *
             *  Need to check if instr. is in a load delay slot and if we
             *  are reading from a load delay target
             */

            stored_halfword = RegFile[target_register]; // Automatically takes least significant 2 bytes of RegFile[target_Register]
            /*
             *  Passing in the address, the data, and the debug flag
             *  Since this is big endian:
             *      First writeByte has a right shift by 8 bits (passes in top 8 bits)
             */
            stored_byte = stored_halfword >> 8; // Upper 8 bits
            writeByte(RegFile[base] + extended_offset, stored_byte, true);
            stored_byte = stored_halfword;      // Lower 8 bits
            writeByte(RegFile[base] + extended_offset + 0x01, stored_byte, true);
            load_delay_slot = false;
            // Vanilla PC update
            //ProgramCounter;
        }
		else if(opcode == 0x2A){
            /*
             *  SWL - Store Word Left
             *  memory[$base + offset] <- RegFile[target_register]
             */
			printf("SWL instruction found!\n");
            base = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset);  // sign extension
            /*
             *  $base = RegFile[base]
             *
             *  Remember this is big endian.
             *  
             *  Similar idea to LWL. We form 
             *  effective address = $base + extended_offset, but this address need 
             *  not be word aligned.
             *  Effective address points to the MSB of a word. 
             *  
             *  We start storing from the most significant bytes of the word in
             *  RegFile[target_register] into the effective address space till we
             *  find a natural word boundary in the memory location.
             */
            uint32_t temp = readWord(RegFile[base] + extended_offset, true);
            temp = temp & 0x0000FFFF;
            temp = temp | (RegFile[target_register] & 0xFFFF0000);
            writeWord(RegFile[base] + extended_offset, temp, true);
            
		}
		else if(opcode == 0x2B){
            /*
             *  SW - Store Word
             *  memory[$base + offset] <- RegFile[target_register]
             */
			printf("SW instruction found!\n");
            base = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset);  // sign extension
            /*
             *  $base = RegFile[base]
             *
             *  Using writeWord(address, data, debug) function.
             *  debug == true - to print a debug message after writeWord
             *
             *  Need to check for load_delay_slot and disable it after
             */
            
            stored_word = RegFile[target_register];
            writeWord(RegFile[base] + extended_offset, stored_word, true);
            
            load_delay_slot = false;
            // Vanilla PC update
            //ProgramCounter;
        }
		else if(opcode == 0x2E){
            /*
             *  SWR - Store Word Right
             *  memory[$base + offset] <- RegFile[target_register]
             *
             */
			printf("SWR instruction found!\n");
            base = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset);  // sign extension
            
            uint32_t temp = readWord(RegFile[base] + extended_offset, true);
            temp = temp & 0xFFFF0000;
            temp = temp | (RegFile[target_register] & 0x0000FFFF);
            writeWord(RegFile[base] + extended_offset, temp, true);
            
        }
		else if(opcode == 0x01){
            source_register = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset) << 2;  // sign extension and left shift by 2 bits
            if(target_register == 0x00){
                /*
                 *  BLTZ - Branch on less than zero
                 */
				printf("BLTZ instruction found!\n");
                if(RegFile[source_register] >> 31 == 1){
                    // branch taken because $rs < 0
                    take_branch = true;
                    branch_PC = ProgramCounter + 0x00000004 + extended_offset;
                }else{
                    // branch not taken because $rs >= 0
                    take_branch = false;
                }
            }
			else if(target_register == 0x01){
                /*
                 *  BGEZ - Branch on greater than or equal to zero
                 */
				printf("BGEZ instruction found!\n");
                if(RegFile[source_register] >> 31 == 0){
                    // branch taken because $rs >= 0
                    take_branch = true;
                    branch_PC = ProgramCounter + 0x00000004 + extended_offset;
                }else{
                    // branch not taken because $rs < 0
                    take_branch = false;
                }
            }
			else if(target_register == 0x10){
                /*
                 *  BLTZAL - Branch on less than zero and link
                 */
				printf("BLTZAL instruction found!\n");
                if(RegFile[source_register] >> 31 == 1){
                    // branch taken because $rs < 0
                    RegFile[31] = ProgramCounter + 0x00000008; // return address
                    take_branch = true;
                    branch_PC = ProgramCounter + 0x00000004 + extended_offset;
                }else{
                    // branch not taken because $rs >= 0
                    take_branch = false;
                }
            }
			else if(target_register == 0x11){
                /*
                 *  BGEZAL - Branch on greater than or equal to zero and link
                 */
				printf("BGEZAL instruction found!\n");
                if(RegFile[source_register] >> 31 == 0){
                    // branch taken because $rs >= 0
                    RegFile[31] = ProgramCounter + 0x00000008; // return address
                    take_branch = true;
                    branch_PC = ProgramCounter + 0x00000004 + extended_offset;
                }else{
                    // branch not taken because $rs < 0
                    take_branch = false;
                }
            }
			else{
                /*
                 *  Exception - there should be no instr like this
                 */
                printf("Opcode = 0x01, Intr is not any of BLTZ, BGEZ, BLTZAL, BGEZAL. Incorrect!\n");
                take_branch = false;
            }
        }
		else if(opcode == 0x02){
            /*
             *  J - jump
             */
			printf("J instruction found!\n");
            pseudo_address = (CurrentInstruction & 0x03FFFFFF) << 2; // shifted up by 2 bits - to generate instruction index
            branch_PC = ((ProgramCounter + 0x00000004) & 0xF0000000) | (pseudo_address);
            take_branch = true;
        }
		else if(opcode == 0x03){
            /*
             *  JAL - Jump and Link
             */
			printf("JAL instruction found!\n");
            RegFile[31] = ProgramCounter + 0x00000008; // return address
            pseudo_address = (CurrentInstruction & 0x03FFFFFF) << 2; // shifted up by 2 bits - to generate instruction index
            branch_PC = ((ProgramCounter + 0x00000004) & 0xF0000000) | (pseudo_address);
            take_branch = true;
        }
		else if(opcode == 0x04){
            /*
             *  BEQ - Branch on equal
             */
			printf("BEQ instruction found!\n");
            source_register = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset) << 2;  // sign extension and left shift by 2 bits
            if(RegFile[source_register] == RegFile[target_register]){
                // branch taken because $rs = $rt
                take_branch = true;
                branch_PC = ProgramCounter + 0x00000004 + extended_offset;
            }else{
                // branch not taken because $rs != $rt
                take_branch = false;
            }
        }
		else if(opcode == 0x05){
            /*
             *  BNE - Branch on not equal
             */
			printf("BNE instruction found!\n");
            source_register = (CurrentInstruction & 0x03E00000) >> 21;
            target_register = (CurrentInstruction & 0x001F0000) >> 16;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset) << 2;  // sign extension and left shift by 2 bits
            if(RegFile[source_register] != RegFile[target_register]){
                // branch taken because $rs != $rt
                take_branch = true;
                branch_PC = ProgramCounter + 0x00000004 + extended_offset;
            }else{
                // branch not taken because $rs == $rt
                take_branch = false;
            }
        }
		else if(opcode == 0x06){
            /*
             *  BLEZ - Branch on less than or equal to zero
             */
			printf("BLEZ instruction found!\n");
            source_register = (CurrentInstruction & 0x03E00000) >> 21;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset) << 2;  // sign extension and left shift by 2 bits
            if(RegFile[source_register] == 0x00000000 || RegFile[source_register] >> 31 == 1){
                // branch taken because $rs <= 0
                take_branch = true;
                branch_PC = ProgramCounter + 0x00000004 + extended_offset;
            }else{
                // branch not taken because $rs > 0
                take_branch = false;
            }
        }
		else if(opcode == 0x07){
            /*
             *  BGTZ - Branch on greater than zero
             */
			printf("BGTZ instruction found!\n");
            source_register = (CurrentInstruction & 0x03E00000) >> 21;
            offset = (CurrentInstruction & 0x0000FFFF);
            extended_offset = sign_extend_16(offset) << 2;  // sign extension and left shift by 2 bits
            if(RegFile[source_register] >> 31 == 0 && RegFile[source_register] != 0x00000000){
                // branch taken because $rs > 0
                take_branch = true;
                branch_PC = ProgramCounter + 0x00000004 + extended_offset;
            }else{
                // branch not taken because $rs <= 0
                take_branch = false;
            }
        }

		RegFile[0] = 0x00000000;
        program_counter_update(&ProgramCounter, &branch_delay_slot, &take_branch, branch_PC); 
        instruction_counter++;
        printf("Instruction #: %d\n", instruction_counter);
    }

	//Close file pointers & free allocated Memory
	closeFDT();
	CleanUp();

	return 0;

}

uint32_t sign_extend_16(uint16_t immediate){
    uint32_t extended_immediate = 0x00000000;
    uint8_t sign = immediate >> 15;
    if(sign == 0x00){
        extended_immediate = immediate | 0x00000000;
    }else{
        extended_immediate = immediate | 0xFFFF0000;
    }
    return extended_immediate;
}

uint32_t zero_extend_16(uint16_t immediate){
    uint32_t extended_immediate = 0x00000000;
    extended_immediate = immediate | 0x00000000;
    return extended_immediate;
}

uint32_t sign_extend_8(uint8_t immediate){
    uint32_t extended_immediate = 0x00000000;
    uint8_t sign = immediate >> 7;
    if(sign == 0x00){
        extended_immediate = immediate | 0x00000000;
    }else{
        extended_immediate = immediate | 0xFFFFFF00;
    }
    return extended_immediate;
}

uint32_t zero_extend_8(uint8_t immediate){
    uint32_t extended_immediate = 0x00000000;
    extended_immediate = immediate | 0x00000000;
    return extended_immediate;
}

void program_counter_update(uint32_t *ProgramCounter, bool *branch_delay_slot, bool *take_branch, uint32_t branch_PC){
    /*
     *  This function handles the program counter change at the end of every
     *  instruction.
     *
     *  Inputs: 
     *      uint32_t ProgramCounter
     *      bool branch_delay_slot = Indicates presence of branch delay slot
     *      uint32_t CurrentInstruction = Needed for jumps
     *  Outputs:
     *      uint32_t NewProgramCounter = Updated program counter
     */
    
    if(*take_branch || *branch_delay_slot){
        if(*take_branch){
            //if this is true then a branch happened
            *ProgramCounter = *ProgramCounter + 0x00000004;
            *branch_delay_slot = true;
            *take_branch = false;
            return;
        }
        else if(*branch_delay_slot){
            printf("IN A DELAY SLOT, WILL JUMP TO %X\n", branch_PC);
            *ProgramCounter = branch_PC;
            *branch_delay_slot = false;
        }
    }

    else{
	    *ProgramCounter = *ProgramCounter + 0x00000004;
    }
    
    // else{
    //     *branch_delay_slot = false;
    // }


    // if(*branch_delay_slot){
    //     printf("IN A DELAY SLOT, WILL JUMP TO %X\n", branch_PC);
    //     *ProgramCounter = branch_PC;
    //     *branch_delay_slot = false;
    // }
	
	

}

void pc_update(uint32_t offset, uint32_t* current_PC, uint32_t* next_PC){

}