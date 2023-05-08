all:
	gcc -o pingpong_scheduler.exe ppos-core-aux.c pingpong-scheduler.c libppos_static.a
	gcc -o pingpong_preemp.exe ppos-core-aux.c pingpong-preempcao.c libppos_static.a
	gcc -o pingpong_contab_prio.exe ppos-core-aux.c pingpong-contab-prio.c libppos_static.a