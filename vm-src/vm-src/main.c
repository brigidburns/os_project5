/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>

struct page_table {
	int fd; 
	char *virtmem; 
	int npages; 
	unsigned char *physmem; 
	int nframes; 
	int *page_mapping; 
	int *page_bits; 
	page_fault_handler_t handler; 
}; 

// Global variables 
struct disk *disk; 
int *frame_table; 
char *algo; 
int counter = 0; 
int page_faults = 0; 
int writes = 0; 
int reads = 0; 

// Initialize frame table
void initialize_frame_table(struct page_table *pt) {
	int num_frames = page_table_get_nframes(pt);  
	int i; 

	// Initialize frame table
	frame_table = (int *)malloc(num_frames*sizeof(int));  

	// Initialize frame table to -1
	for (i = 0; i < num_frames; i++) {
		frame_table[i] = -1; 
	}
}

// Function to find a random page to evict
int get_rand_num(struct page_table *pt) {

	int upper = page_table_get_nframes(pt); 
	int rand_page; 

	// Generate the random number
	rand_page = (rand() % upper); 
	return rand_page; 
}

void evict_page(struct page_table *pt, int page_to_evict) {
	int frame_involved, bits; 

	page_table_get_entry(pt, page_to_evict, &frame_involved, &bits); 

	if ((bits & PROT_WRITE)) {
		disk_write(disk, page_to_evict, &pt->physmem[frame_involved*PAGE_SIZE]); 
		writes++; 
	} 

	page_table_set_entry(pt, page_to_evict, frame_involved, 0);
}

void page_fault_handler( struct page_table *pt, int page )
{
	// Count page faults 
	page_faults++; 

	int frame, bits, i, num_frames = page_table_get_nframes(pt); 
	page_table_get_entry(pt, page, &frame, &bits); 

	// If a page is not in memory
	if ((bits & PROT_READ) == 0) { 
		// Search for an unused frame
		int free_frame = -1; 
		for (i = 0; i < num_frames; i++) {
			if (frame_table[i] == -1) { // Found an unused frame! Yay!
				free_frame = i; // Keep the index of the free frame
				break; 
			}
		}

		if (free_frame != -1) { // Page is not in memory, and there is an unused frame
			frame_table[free_frame] = page; 
			page_table_set_entry(pt, page, free_frame, PROT_READ); 
			disk_read(disk, page, &pt->physmem[free_frame*PAGE_SIZE]);
			reads++; 
		}
		else { // Page is not in memory, and no free frames
			int frame_to_evict, page_to_evict; 
			if (strcmp(algo, "fifo") == 0) { // Fifo
				// Find the frame to evict
				frame_to_evict = counter;

				// Find the associated page to evict
				page_to_evict = frame_table[frame_to_evict]; 
				
				// Evict the page
				evict_page(pt, page_to_evict);  

				// Load the new page
				frame_table[frame_to_evict] = page; 
				page_table_set_entry(pt, page, frame_to_evict, PROT_READ); 
				disk_read(disk, page, &pt->physmem[frame_to_evict*PAGE_SIZE]); 
				reads++; 
				counter = (counter+1) % num_frames;
			}
			else if (strcmp(algo, "rand") == 0) { // Rand
				// Find the frame to evict
				frame_to_evict = get_rand_num(pt); 

				// Find the associated page to evict
				page_to_evict = frame_table[frame_to_evict]; 

				// Evict the page
				evict_page(pt, page_to_evict); 

				// Load the new page
				frame_table[frame_to_evict] = page; 
				page_table_set_entry(pt, page, frame_to_evict, PROT_READ); 
				disk_read(disk, page, &pt->physmem[frame_to_evict*PAGE_SIZE]); 
				reads++; 
			}
			else { // Custom
				int first_frame = 0; 
				int second_frame = (page_table_get_nframes(pt) - 1); 
				int first_page, second_page; 

				first_page = frame_table[first_frame]; 
				second_page = frame_table[second_frame]; 

				evict_page(pt, first_page); 

				frame_table[first_frame] = page; 
				page_table_set_entry(pt, page, first_frame, PROT_READ); 
				disk_read(disk, page, &pt->physmem[first_frame*PAGE_SIZE]); 
				reads++;

				evict_page(pt, second_page); 
				frame_table[second_frame] = -1; 
			}
		}
	}
	else { // Page is in memory 
		page_table_set_entry(pt, page, frame, PROT_READ|PROT_WRITE);
		return; 
	}
}

int main( int argc, char *argv[] )
{
	srand(time(NULL)); 

	if(argc!=5) {
		printf("use: virtmem <npages> <nframes> <rand|fifo|custom> <alpha|beta|gamma|delta>\n");
		return 1;
	}

	int npages = atoi(argv[1]);
	int nframes = atoi(argv[2]);
	algo = argv[3]; 
	const char *program = argv[4];

	disk = disk_open("myvirtualdisk",npages); 
	if(!disk) {
		fprintf(stderr,"couldn't create virtual disk: %s\n",strerror(errno));
		return 1;
	}


	struct page_table *pt = page_table_create( npages, nframes, page_fault_handler );
	if(!pt) {
		fprintf(stderr,"couldn't create page table: %s\n",strerror(errno));
		return 1;
	}

	// Initialize the frame table
	initialize_frame_table(pt); 

	unsigned char *virtmem = page_table_get_virtmem(pt);

	unsigned char *physmem = page_table_get_physmem(pt);

	if(!strcmp(program,"alpha")) {
		alpha_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"beta")) {
		beta_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"gamma")) {
		gamma_program(virtmem,npages*PAGE_SIZE);

	} else if(!strcmp(program,"delta")) {
		delta_program(virtmem,npages*PAGE_SIZE);

	} else {
		fprintf(stderr,"unknown program: %s\n",argv[4]);
		return 1;
	}

	printf("%d\n", page_faults); 
	printf("%d\n", reads); 
	printf("%d\n", writes); 

	page_table_delete(pt);
	disk_close(disk);
	free(frame_table); 

	return 0;
}
