	#include <assert.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdint.h>
	#include <string.h>

	#include "disk.h"
	#include "fs.h"

	/* TODO: Phase 1 */

	int mount = 0;
	struct  __attribute__((__packed__)) superBlock {
		uint8_t	 	signature[8];
		uint16_t 	total_Block_counter;
		uint16_t 	Root_Dir;
		uint16_t 	Data_Block;
		uint16_t 	Amout_Data_Block;
		uint8_t 	Fat_block;
		uint8_t		padding [4079];
	};

	struct  __attribute__((__packed__)) fatBlock {
		uint16_t 	*Fat_Block_index;
		
	};

	struct  __attribute__((__packed__)) rootDir {
		uint8_t  	Filename[FS_FILENAME_LEN];
		uint32_t 	file_Size;
		uint16_t 	index;
		uint8_t 	padding[10];
	};

	struct  __attribute__((__packed__)) file_descriptor {
		uint8_t 		Filename[FS_FILENAME_LEN];
		int				fdi;
		size_t			os;
	};



	struct superBlock sb;
	struct fatBlock Fb; 
	struct rootDir rd[FS_FILE_MAX_COUNT];
	struct file_descriptor fileD[FS_OPEN_MAX_COUNT];

	int fs_mount(const char *diskname)
	{
		int open = block_disk_open(diskname);
		if(open == -1){
			return -1;
		}
		if(block_read(0, &sb) == -1){
			return -1;
		}
		if(strcmp("ECS150FS", (char*)sb.signature) == -1){
			return -1;
		}
		if(sb.total_Block_counter != block_disk_count()){
			return -1;
		}

		Fb.Fat_Block_index = malloc(BLOCK_SIZE *(sb.Fat_block)* 2);
		for(int i = 1; i <= sb.Fat_block; i ++){
			block_read(i, &Fb.Fat_Block_index[i-1]);	
		}

		block_read(sb.Root_Dir,&rd);
		mount = 1;
		return 0;


	}

	int fs_umount(void)
	{
		if (mount == 0)	
			return -1;
		if(block_read(0, &sb) == -1){
			return -1;
		}
		
		for(int i = 1; i <= sb.Fat_block; i++){
			block_write(i, &Fb.Fat_Block_index[i-1]);	
		}
		block_write(sb.Root_Dir,&rd);
		int close = block_disk_close();
		if(close == -1){
			return -1;
		}
		else{
			mount =0;
			return 0;
		} 
		

	}

	int fs_info(void)
	{
		/* TODO: Phase 1 */
		if(mount == 0){
			return -1;
		}
		printf("FS Info:\n");
		printf("total_blk_count=%d\n", sb.total_Block_counter);
		printf("fat_blk_count=%d\n", sb.Fat_block);
		printf("rdir_blk=%d\n", sb.Root_Dir);
		printf("data_blk=%d\n", sb.Data_Block);
		printf("data_blk_count=%d\n", sb.Amout_Data_Block);
		
		int zerosFat=0;
		int zerosRD = 0;
		for (int  i =0; i< sb.Amout_Data_Block; i++ ){
		
		if(Fb.Fat_Block_index[i] == 0)
				zerosFat++;
		}
		
		printf("fat_free_ratio=%d/%d\n", zerosFat, sb.Amout_Data_Block);
		
		for (int  i =0; i< 128 ; i++ ){
			if(rd[i].Filename[0] == 0)
				zerosRD++;
		}
		printf("rdir_free_ratio=%d/%d\n",zerosRD,FS_FILE_MAX_COUNT);
		
		return 0;
		
		}
	
			

	int fs_create(const char *filename) 
	{
	
		if(strlen(filename) >= FS_FILENAME_LEN || mount == 0)
			return -1;
		for(int i =0; i<168; i++){
		if(strcmp(filename, (char*)rd[i].Filename) == 0) {
            return 0;
        }
				
		if(rd[i].Filename[0] == '\0'){
				memcpy(rd[i].Filename, filename, FS_FILENAME_LEN);
				rd[i].file_Size = 0;
				rd[i].index = 0xFFFF;
				break;
				}
		}
		
			return 0;
		
	}

	int fs_delete(const char *filename)
	{
		/* TODO: Phase 2 */
		if(strlen(filename) > FS_FILENAME_LEN || mount == 0)
			return -1;
		
		int delete = 0;
		int fat_index = 0;
		for(int i =0; i < FS_FILE_MAX_COUNT; i++){

			if(strcmp(filename, (char*) rd[i].Filename)==0){
				rd[i].Filename[0] = '\0';
				rd[i].file_Size = 0;
				fat_index =rd[i].index;
				rd[i].index = 0xFFFF;
				delete++;
				break;
			}
		}
		uint16_t temp=0;
		if (delete == 1){
			while(fat_index != 0xFFFF){
				temp = Fb.Fat_Block_index[fat_index];
				Fb.Fat_Block_index[fat_index] = 0;
				fat_index = temp;
			}
			return 0;
		}	
		else 
			return -1;
		
	}

	int fs_ls(void)
	{
		/* TODO: Phase 2 */
		if(mount == 0)
			return -1;
		printf("FS ls \n");
		for(int i =0; i< FS_FILE_MAX_COUNT; i++){
			if(rd[i].Filename[0] != '\0'){
				printf("File name %s, File size %d, First Data Block %d\n", 
				rd[i].Filename, rd[i].file_Size, rd[i].index);
			}

		}
		return 0;

	}


	int fs_open(const char *filename)
	{
		/* TODO: Phase 3 */
		if(mount == 0 || strlen(filename) > FS_FILENAME_LEN )
			return -1;
		
		int found = 0;
		for(int i =0; i< FS_FILE_MAX_COUNT; i++){
			if(strcmp((char*)rd[i].Filename, filename)==0){
				found ++;
				break;
			}
		}
		if(found == 0)
			return -1;

		for(int i =0; i < FS_OPEN_MAX_COUNT; i++){

			if(rd[i].Filename[0]== '\0'){
				memcpy((char*)fileD[i].Filename, filename, FS_FILENAME_LEN);
				fileD[i].os = 0;
				fileD[i].fdi = i;
				return i;
			}
	
		}
		return -1;

	}

	int fs_close(int fd)
	{
		/* TODO: Phase 3 */
		if(mount == 0)
			return -1;
		if(fd > 31 || fd < 0)
			return -1;
		if(strlen( (char*)fileD[fd].Filename) == 0)
			return -1;
		fileD[fd].Filename[0] ='\0';
		fileD[fd].fdi = 0;
		return 0;
		
		
	}

	int fs_stat(int fd)
	{
		/* TODO: Phase 3 */
		if(mount == 0)
			return -1;
		if(fd > 31 || fd < 0)
			return -1;
		if(strlen( (char*)fileD[fd].Filename) == 0)
			return -1;
		

		for(int i =0; i < FS_FILE_MAX_COUNT; i++){
			if(strcmp((char*)fileD[fd].Filename,(char*)rd[i].Filename)== 0){
				return rd[i].file_Size;
			}
		}
		return -1;
	}

	int fs_lseek(int fd, size_t offset)
	{
		/* TODO: Phase 3 */

		if(mount == 0 || fd > FS_OPEN_MAX_COUNT || fd < 0)
			return -1;
		
		if(strlen((char*)fileD[fd].Filename) == 0){
			return -1;
		}	
		fileD[fd].os = offset;
		return 0;
	
	}

	int helper(){
		for (int  i =0; i< sb.Amout_Data_Block; i++ ){
		if(Fb.Fat_Block_index[i] == 0)
				return i;
		}
		return -1;
	}

	uint16_t next_fat() {
		
		for (uint16_t i= 1; i < sb.Amout_Data_Block; i++){
			if (Fb.Fat_Block_index[i] == 0){
			Fb.Fat_Block_index[i] = 0xFFFF; 
			return i;
			}
	}
		return -1;
}

int fs_write(int fd, void *buf, size_t count)
	{
		/* TODO: Phase 4 */
		
		if(mount ==0 || fd >FS_OPEN_MAX_COUNT || fd < 0)
			return -1;
		if(fileD[fd].Filename[0] == '\0')
			return -1;

		int f_index =0;
		int f=0;
		for(int i =0; i< FS_FILE_MAX_COUNT; i++){
			if(strcmp((char*)rd[i].Filename, (char*)fileD[fd].Filename)==0){
				f_index= rd[i].index;
				f =i;
				break;
			}
		}

		int write = 0;
		void *temp_buffer = (void*)malloc(BLOCK_SIZE);
		int starting_block = fileD[fd].os / BLOCK_SIZE;  
		int os =  fileD[fd].os % BLOCK_SIZE;
		if(f_index != 0xFFFF)
			block_read(f_index + sb.Data_Block + starting_block, temp_buffer);
		int size = strlen((char*)temp_buffer);
		if(os == 0  && size != 0)
			rd[f].file_Size = rd[f].file_Size-size;
		for(int i =0; i < count; i++){

		if(fs_stat(fd) > 0 && rd[f].index == 0xFFFF){
			rd[f].index = next_fat();
			f_index = rd[f].index;
		}
	
		if(os >= BLOCK_SIZE){
			block_write(f_index + sb.Data_Block + starting_block, temp_buffer);
			uint16_t temp =0;
			temp = next_fat();
			Fb.Fat_Block_index[f_index] =temp;
			f_index = temp;
			os =0;
		}
			rd[f].file_Size = rd[f].file_Size+1;
			memcpy(temp_buffer+os, buf+i, 1);
			os++;
			write++;
		}
		block_write(f_index + sb.Data_Block + starting_block, temp_buffer);
		return write;

	}


int fs_read(int fd, void *buf, size_t count)
	{
		/* TODO: Phase 4 */
	
		if(mount ==0 || fd >FS_OPEN_MAX_COUNT || fd < 0)
			return -1;
		if(fileD[fd].Filename[0] == '\0')
			return -1;

		uint16_t f_index =0;
		for(int i =0; i< FS_FILE_MAX_COUNT; i++){
			if(strcmp((char*)rd[i].Filename, (char*)fileD[fd].Filename)==0){
				f_index= rd[i].index;
				break;
			}
		}
		int read = 0;
		void *temp_buffer = (void*)malloc(BLOCK_SIZE);
		int starting_block = fileD[fd].os / BLOCK_SIZE;  

		


		int os =  fileD[fd].os % BLOCK_SIZE;
		if(f_index == sb.Data_Block)
			block_read(f_index + 0 + starting_block, temp_buffer);

		block_read(f_index + sb.Data_Block + starting_block, temp_buffer);
		for(int i =0; i < count; i++){
			if(i == fs_stat(fd))
				return read;
			if(BLOCK_SIZE <= os){
				if(Fb.Fat_Block_index[f_index] != 0xFFFF){
					f_index = Fb.Fat_Block_index[f_index];
				}
				else
					return read;
				block_read(f_index + sb.Data_Block + starting_block, temp_buffer);
				os =0;
			}
			memcpy(buf + i, temp_buffer+os, 1);
			read++;
			os++;
		}
		return read;

	}

