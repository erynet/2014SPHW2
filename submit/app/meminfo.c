#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>
#include <linux/sched.h>
#include <linux/pid.h>
#include <asm/pgtable.h>

static int mypid = -1; //지정되지 않을 경우에 대비해서 일단 -1 로 기본값을 넣는다.
module_param(mypid, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IXUSR | S_IXGRP); // 모듈의 변수로서 등록
MODULE_PARM_DESC(mypid, "A Procee ID Variable"); // 변수 설명을 넣어준다. 필수 요소는 아니지만

static int __init hw2_init(void){
	int input_flag = -1;
	int data_flag = 1;
	int pid = mypid;
	unsigned long code_start;
	unsigned long addr;
	pgd_t *pgd; //global
	pud_t *pud; //upper
	pmd_t *pmd; //middle
	pte_t *pte; //table
	unsigned long bit_flag; // flag 정보를 얻어와서 담을 변수를 만든다.
	
	struct thread_info *thread; 
	struct mm_struct *mm;
	struct task_struct *task;
	struct vm_area_struct *mmap;
	
	if(pid == -1){
		//모듈을 실행할때 제대로 인자를 않넣어준 경우 기본값은 -1로 값이 남아있을 것이므로 이때는 사용법 안내 메시지를 출력하고 종료한다
		printk(KERN_INFO "USAGE : insmod meminfo mypid=1234\n");
		return 0;
	}
	
	thread = current_thread_info(); //현재의 쓰레드 포인터를 얻어온다
	task = thread->task; //태스크 스트럭트의 첫번째 포인터를 얻어온다 (밑에서 반복문을 돌기 위해 ..)

	for_each_process(task){ // 각 태스크를 돈다
		if(task->pid == pid){ // 만약 일치하는 pid 를 찾은 경우
		
			if(task->mm == NULL){ // 찾았는데도 불구하고 mm 구조체가 없는 경우 
				input_flag = -2;
				break;
			}
			mm = task->mm;
			if(mm->mmap == NULL){ // 찾았는데도 불구하고 mmap 안에 mm 객체가 없는 경우
				input_flag = -3;
				break;
			}
			mmap = mm->mmap;

			printk(KERN_INFO "\n********************************************************************************\n");
			printk(KERN_INFO "Student ID: 0640353      Name: DONG HOE, LEE\n");
			printk(KERN_INFO "Virtual Memory Address Information of Process (%s : %7d)\n",task->comm,task->pid);
			printk(KERN_INFO "********************************************************************************\n");

			if(pid <= 10 || mmap->vm_start >= mm->end_code || mmap->vm_end <= mm->start_code){
				data_flag = -1; //만약 입력된 pid 가 10 이하라면, 커널이 운영하는 특수 프로세스이므로
			}

			if(data_flag == -1){
				printk(KERN_INFO "0x%08lx - 0x%08lx : Shared Libraries Area, %d Page(s)\n", mmap->vm_start, mmap->vm_end, (int)((mmap->vm_end - mmap->vm_start)/PAGE_SIZE));
			}
			else{
				printk(KERN_INFO "0x%08lx - 0x%08lx : Code Area, %d Page(s)\n", mmap->vm_start, mmap->vm_end, (int)((mmap->vm_end - mmap->vm_start)/PAGE_SIZE));
			}
			code_start = mmap->vm_start;
			
			while((mmap = mmap->vm_next) != NULL){
				
				if(data_flag != -1 && mmap->vm_start >= mm->start_stack){
					printk(KERN_INFO "0x%08lx - 0x%08lx : Stack Area, %d Page(s)\n", mmap->vm_start, mmap->vm_end, (int)((mmap->vm_end - mmap->vm_start)/PAGE_SIZE));
					continue;
				}

				if(data_flag == 1 && ((mmap->vm_next)->vm_start == mm->start_brk || (mmap->vm_next)->vm_end == mm->brk)){
					printk(KERN_INFO "0x%08lx - 0x%08lx : BSS Area, %d Page(s)\n", mmap->vm_start, mmap->vm_end, (int)((mmap->vm_end - mmap->vm_start)/PAGE_SIZE));
					mmap = mmap->vm_next;
					printk(KERN_INFO "0x%08lx - 0x%08lx : Heap Area, %d Page(s)\n", mmap->vm_start, mmap->vm_end, (int)((mmap->vm_end - mmap->vm_start)/PAGE_SIZE));					
					data_flag = 0;
					continue;
				}
				
				if(data_flag == 1){
					printk(KERN_INFO "0x%08lx - 0x%08lx : Data Area, %d Page(s)\n", mmap->vm_start, mmap->vm_end, (int)((mmap->vm_end - mmap->vm_start)/PAGE_SIZE));
				}
				else{
					printk(KERN_INFO "0x%08lx - 0x%08lx : Shared Libraries Area, %d Page(s)\n", mmap->vm_start, mmap->vm_end, (int)((mmap->vm_end - mmap->vm_start)/PAGE_SIZE));
				}
			
			}

			printk(KERN_INFO "********************************************************************************\n");
			printk(KERN_INFO "1 Level Paging: Page Directory Entry Information\n");
			printk(KERN_INFO "********************************************************************************\n");
			printk(KERN_INFO "PGD      Base Address           : 0x%lx\n",(unsigned long)mm->pgd);
			pgd = pgd_offset(mm,code_start); //mm_struct 와 code_start 주소를 통해서 pgd 주소를 얻어온다.
			printk(KERN_INFO "code     PGD Address            : 0x%lx\n",(unsigned long)pgd);
			addr = (pgd)->pgd;
			printk(KERN_INFO "         PGD Value              : 0x%lx\n",(unsigned long)addr);
			printk(KERN_INFO "         +PFN Address           : 0x%09lx\n",(unsigned long)addr/PAGE_SIZE);
			
			bit_flag = 0x80; // 비트플래그를  1000 0000 로 설정한다
			if((bit_flag & addr) == bit_flag) // page_size bit flag. 1000 0000
				printk(KERN_INFO "         +Page Size             : 4MB\n");
			else
				printk(KERN_INFO "         +Page Size             : 4KB\n");
			
			bit_flag = bit_flag>>2;
			if((bit_flag & addr) == bit_flag) // accessed bit flag. 0010 0000
				printk(KERN_INFO "         +Accessed Bit          : 1\n");
			else
				printk(KERN_INFO "         +Accessed Bit          : 0\n");

			bit_flag = bit_flag>>1;
			if((bit_flag & addr) == bit_flag) // cache disable bit flag. 0001 0000
				printk(KERN_INFO "         +Cache Disable Bit     : Disable\n");
			else
				printk(KERN_INFO "         +Cache Disable Bit     : Enable\n");

			bit_flag = bit_flag>>1;
			if((bit_flag & addr) == bit_flag) // write-through bit flag 0000 1000
				printk(KERN_INFO "         +Page Write-Through    : Write-Through\n");
			else
				printk(KERN_INFO "         +Page Write-Through    : Write-Back\n");

			bit_flag = bit_flag>>1;
			if((bit_flag & addr) == bit_flag) // user/supervisor bit flag 0000 0100
				printk(KERN_INFO "         +User/Supervisor Bit   : User\n");
			else
				printk(KERN_INFO "         +User/Supervisor Bit   : Supervisor\n");

			bit_flag = bit_flag>>1;
			if((bit_flag & addr) == bit_flag) // Read/Write bit flag 0000 0010
				printk(KERN_INFO "         +Read/Write Bit        : Read/Write\n");
			else
				printk(KERN_INFO "         +Read/Write Bit        : Read-Only\n");
			
			bit_flag = bit_flag>>1;
			if((bit_flag & addr) == bit_flag) // Present bit flag 0000 0001
				printk(KERN_INFO "         +Page Present Bit      : 1\n");
			else
				printk(KERN_INFO "         +Page Present Bit      : 0\n");

			printk(KERN_INFO "********************************************************************************\n");
			printk(KERN_INFO "2 Level Paging: Page Upper Directory Entry Information\n"); // second level paging information.
			printk(KERN_INFO "********************************************************************************\n");

			pud = pud_offset(pgd, code_start); // get pud Address.
			printk(KERN_INFO "code     PUD Address            : 0x%lx\n",(unsigned long)pud); 
			addr = pud->pud; // get pud value.

			printk(KERN_INFO "         PUD Value              : 0x%lx\n",(unsigned long)addr);
			printk(KERN_INFO "         +PFN Address           : 0x%09lx\n",(unsigned long)addr/PAGE_SIZE); // print PFN Address.
			printk(KERN_INFO "********************************************************************************\n");
			printk(KERN_INFO "3 Level Paging: Page Middle Directory Entry Information\n"); // third level paging information.
			printk(KERN_INFO "********************************************************************************\n");

			pmd = pmd_offset(pud, code_start); // get pmd Address.
			printk(KERN_INFO "code     PMD Address            : 0x%lx\n",(unsigned long)pmd);
			addr = (pmd)->pmd; // get pmd value.

			printk(KERN_INFO "         PMD Value              : 0x%lx\n",(unsigned long)addr);
			printk(KERN_INFO "         +PFN Address           : 0x%09lx\n",(unsigned long)addr/PAGE_SIZE); // print PFN Address.
			printk(KERN_INFO "********************************************************************************\n");
			printk(KERN_INFO "4 Level Paging: Page Table Entry Information\n"); // fourth level paging information.
			printk(KERN_INFO "********************************************************************************\n");

			pte = pte_offset_kernel(pmd,code_start); // get pte Address
			printk(KERN_INFO "code     PTE Address            : 0x%lx\n",(unsigned long)pte);
			addr = (pte)->pte; // get pte value.
			printk(KERN_INFO "         PTE Value              : 0x%lx\n",(unsigned long)addr);
			printk(KERN_INFO "         +Page Base Address     : 0x%09lx\n",(unsigned long)addr/PAGE_SIZE); // print Page Base Address.

			bit_flag = 0x40; // set bit_flag 0100 0000
			if((bit_flag & addr) == bit_flag) // Dirty bit flag 0100 0000
				printk(KERN_INFO "         +Dirty Bit             : 1\n");
			else
				printk(KERN_INFO "         +Dirty Bit             : 0\n");

			bit_flag = bit_flag>>1; 
			if((bit_flag & addr) == bit_flag) // Accessed bit flag 0010 0000
				printk(KERN_INFO "         +Accessed Bit          : 1\n");
			else
				printk(KERN_INFO "         +Accessed Bit          : 0\n");

			bit_flag = bit_flag>>1;
			if((bit_flag & addr) == bit_flag) // cache disable bit flag 0001 0000
				printk(KERN_INFO "         +Cache Disable Bit     : Disable\n");
			else
				printk(KERN_INFO "         +Cache Disable Bit     : Enable\n");

			bit_flag = bit_flag>>1;
			if((bit_flag & addr) == bit_flag) // write-through bit flag 0000 1000
				printk(KERN_INFO "         +Page Write-Through    : Write-Through\n");
			else
				printk(KERN_INFO "         +Page Write-Through    : Write-Back\n");

			bit_flag = bit_flag>>1;
			if((bit_flag & addr) == bit_flag) // user/supervisor bit flag 0000 0100
				printk(KERN_INFO "         +User/Supervisor Bit   : User\n");
			else
				printk(KERN_INFO "         +User/Supervisor Bit   : Supervisor\n");

			bit_flag = bit_flag>>1;
			if((bit_flag & addr) == bit_flag) // read/write bit flag 0000 0010
				printk(KERN_INFO "         +Read/Write Bit        : Read/Write\n");
			else
				printk(KERN_INFO "         +Read/Write Bit        : Read-Only\n");

			bit_flag = bit_flag>>1;
			if((bit_flag & addr) == bit_flag) // presend bit flag 0000 0001
				printk(KERN_INFO "         +Page Present Bit      : 1\n");
			else
				printk(KERN_INFO "         +Page Present Bit      : 0\n");

			code_start = code_start - (unsigned long)(code_start / PAGE_SIZE)*PAGE_SIZE;
			addr = (unsigned long)(addr / PAGE_SIZE)*PAGE_SIZE; // 물리 주소의 시작점을 얻어옴

			printk(KERN_INFO "********************************************************************************\n");
			printk(KERN_INFO "Start of Physical Address       : 0x%lx\n",(unsigned long)(code_start + addr));
			printk(KERN_INFO "********************************************************************************\n\n");

			input_flag = 1;
			break;
		}
	}
	if(input_flag == -1) // 주어진 pid 에 해당하는 프로세스가 없는 경우
	{
		printk(KERN_INFO "\n********************************************************************************\n");
		printk(KERN_INFO "Student ID: 0640353      Name: DONG HOE, LEE\n");
		printk(KERN_INFO "There is no process id %d\n",mypid);
		printk(KERN_INFO "Goodbye, meminfo");
		printk(KERN_INFO "********************************************************************************\n\n");
	}
	else if(input_flag == -2) // mm struct 가 없을때 출력해야 하는 부분
	{
		printk(KERN_INFO "\n********************************************************************************\n");
		printk(KERN_INFO "Student ID: 0640353      Name: DONG HOE, LEE\n");
		printk(KERN_INFO "There is no mm struct in process id %d\n",mypid);
		printk(KERN_INFO "Goodbye, meminfo");
		printk(KERN_INFO "********************************************************************************\n\n");
	}
	else if(input_flag == -3) // vm_area_struct 가 없을때 출력해야 하는 부분
	{
		printk(KERN_INFO "\n********************************************************************************\n");
		printk(KERN_INFO "Student ID: 0640353      Name: DONG HOE, LEE\n");
		printk(KERN_INFO "There is no vm_area_struct in mm_struct about process id %d\n",mypid);
		printk(KERN_INFO "Goodbye, meminfo");
		printk(KERN_INFO "********************************************************************************\n\n");
	}
	
	return 0;
}

static void __exit hw2_exit(void) // 모듈 제거 시 반환 작업을 수행하는 코드
{}
 
module_init(hw2_init);
module_exit(hw2_exit);

MODULE_LICENSE("GPL");

MODULE_AUTHOR("0640353-ldh");
MODULE_DESCRIPTION("Process Paging Address Info!!!");