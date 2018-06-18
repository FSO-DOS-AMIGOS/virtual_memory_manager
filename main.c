#include <stdio.h>
#include <stdint.h>

uint8_t tlb_page_number[16] = {0};
uint8_t tlb_frame_number[16]= {0};
uint8_t page_table[256] = {0};
uint8_t number_frames_used = 0;
uint8_t last_updated_in_tlb = 0;
uint8_t physical_memory[65536] = {0};
unsigned int page_error = 0;
unsigned int tlb_success = 0;
unsigned int memory_access_counter = 0;

uint8_t verify_tlb(uint8_t page_number);
uint8_t verify_page_table(uint8_t page_number);
void update_tlb(uint8_t page_number, uint8_t frame_number);
void update_physical_memory(uint8_t page_number,uint8_t frame_number);
uint8_t read_physical_memory(uint16_t memory_position);
int is_initialized(void);

FILE *bfptr; //backing store memory file pointer

int main(int argc, char *argv[]){

	if(argc < 2){
		printf("Use: ./proj2.out addresses.txt\n");
		return -1;
	}

	int read_value;
	uint16_t logical_addr;
	uint8_t page_number;
	uint8_t offset;
	uint8_t frame_number;
	uint16_t i;
	uint16_t memory_position;
	uint8_t data_read;

	double error_page_rate;
	double tlb_success_rate;

	FILE *fptr;
	fptr = fopen(argv[1],"r");
	if(fptr == NULL){
		printf("Error opening file\n");
		return -1;
	}
	bfptr = fopen("BACKING_STORE.bin","rb");
	if(bfptr == NULL){
		printf("Error opening file\n");
		return -1;
	}


	while(fscanf(fptr,"%d",&read_value) != EOF){
		memory_access_counter++;
		printf("read value: %d ",read_value);
		logical_addr = 0xFFFF & read_value;
		page_number = (0xFF00 & logical_addr) >> 8;
		offset = 0x00FF & logical_addr;
		printf("logical addr: %u page number: %u offset: %u\n",logical_addr,page_number,offset);
		frame_number = verify_tlb(page_number);
		printf("Frame number found: %u \n", frame_number);
		memory_position = frame_number*256 + offset;
		printf("Memory position found: %u\n", memory_position);
		data_read = read_physical_memory(memory_position);
		printf("Data read: %u\n", data_read);
		printf("\n");

	}

	error_page_rate = page_error/(memory_access_counter*1.0);
	tlb_success_rate = tlb_success/(memory_access_counter*1.0);

	printf("Error page rate: %.2lf\n", error_page_rate*100);
	printf("Tlb success rate: %.2lf\n", tlb_success_rate*100);

/*
	for(i=0; i<256; i++){
		printf("Page number: %u Frame number: %u \n", i, page_table[i]);
	}

*/
	fclose(fptr);
	fclose(bfptr);
	return 0;
}

uint8_t verify_tlb(uint8_t page_number){
	uint8_t i;
	printf("Verifying for page number in TLB...\n");
	if(is_initialized() == 1){
		for(i=0; i<16; i++){

			//printf("Position: %u Page Number: %u Frame Number: %u \n", i, tlb_page_number[i],tlb_frame_number[i]);

			if(tlb_page_number[i] == page_number){
				tlb_success++;
				printf("Found frame %u in TLB.\n",tlb_frame_number[i]);
				return tlb_frame_number[i];
			}
		}
	}
	printf("Page number not found in TLB.\n");
	printf("Opening the page table...\n");
	return verify_page_table(page_number);
}


uint8_t verify_page_table(uint8_t page_number){
	uint8_t i;
	uint8_t j;
	for(i=0; i<256; i++){
		
		/*There's no frame used. We have to initialize the page table*/
		if((i == page_number) && (number_frames_used == 0)){
			printf("Page not initialized in memory yet. Page error generated.\n");
			
			printf("Updating physical memory with the new frame...\n");
			update_physical_memory(page_number, number_frames_used);
			printf("Updated.\n");
			//printf("The page table was not initialized and will be initialized.\n");
			page_table[i] = number_frames_used;
			number_frames_used++;

			/*update the tlb*/
			printf("Updating the tlb with the new page number...\n");
			
			update_tlb(page_number,page_table[i]);
			
			printf("Done.\n");
			return page_table[i];	
		}
		/*There's no frame for the page we're searching now*/
		else if(i == page_number && (page_table[i] == 0)){
			page_error++;
			printf("Page not initialized in memory yet. Page error generated.\n");
			//printf("There's a frame free and the page table will be updated.\n");
			printf("Updating physical memory with the new frame...\n");
			update_physical_memory(page_number, number_frames_used);
			printf("Updated.\n");
			page_table[i] = number_frames_used;
			number_frames_used++;
			
			printf("Updating the tlb with the new page number...\n");
			update_tlb(page_number,page_table[i]);
			printf("Done.\n");
			return page_table[i];
		}
		/*There's already a frame for the page we're searching*/
		else if(i == page_number){
			printf("Page found in memory.\n");
			printf("Updating the tlb with the new page number...\n");
			update_tlb(page_number,page_table[i]);
			printf("Done.\n");
			return page_table[i];
		}
	}
}

void update_tlb(uint8_t page_number, uint8_t frame_number){

	/*Initializing the tlb*/
	if((frame_number == 0) && (last_updated_in_tlb == 0)){
		tlb_page_number[0] = page_number;
		tlb_frame_number[0] = frame_number;
		printf("TLB updated: Position %u Page Number: %u Frame Number: %u\n", last_updated_in_tlb, tlb_page_number[0], tlb_frame_number[0]);
		last_updated_in_tlb++;
	}
	else{
		tlb_page_number[last_updated_in_tlb] = page_number;
		tlb_frame_number[last_updated_in_tlb] = frame_number;
		printf("TLB updated: Position %u Page Number: %u Frame Number: %u\n", last_updated_in_tlb, tlb_page_number[last_updated_in_tlb], tlb_frame_number[last_updated_in_tlb]);
		last_updated_in_tlb = (last_updated_in_tlb + 1) % 16;
	}

}

void update_physical_memory(uint8_t page_number,uint8_t frame_number){

	uint16_t i = 0;
	fseek(bfptr, page_number*256, SEEK_SET);
	for(i=0; i<256; i++){
		fread(&physical_memory[frame_number*256 + i], 1, 1, bfptr);
	}
}

uint8_t read_physical_memory(uint16_t memory_position){
	uint8_t data_read;
	data_read = physical_memory[memory_position];
	return data_read;
}

int is_initialized(void){
	
	int true1 = (tlb_page_number[0] == 0) && (tlb_frame_number[0] == 0);
	int true2 = (tlb_page_number[1] == 0) && (tlb_frame_number[1] == 0);

	if(true1 == 1 && true2 == 1){
		return 0;
	}
	else{
		return 1;
	}

}