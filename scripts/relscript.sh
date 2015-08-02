#!/usr/bin/perl

my $skip_section = 0;

sub strasbourg_filter
{
	my $line = $_[0];

	if ($skip_section == 1) {
		if ($line =~ /^#\ /) {
			if ($line !~ /^# CONFIG/) {
				$skip_section = 0;
				print "#\n";
			}
		}

		if ($skip_section) {
#			print "SKIPPED: $line";
			next;
		}
	}

	if ($line =~ /CONFIG_OPROFILE_OMAP_GPTIMER=y/) {
		next;
	}

#	if ($line =~ /CONFIG_KALLSYMS_ALL=y/) {
#		next;
#	}

	if ($line =~ /CONFIG_OPROFILE=m/) {
		print "# CONFIG_OPROFILE is not set\n";
		next;
	}

	if ($line =~ /CONFIG_TASKSTATS=y/) {
		print "# CONFIG_TASKSTATS is not set\n";
		next;
	}

	if ($line =~ /CONFIG_TRACEPOINTS=y/) {
		next;
	}

	if ($line =~ /CONFIG_PM_DEBUG=y/) {
		print "# CONFIG_PM_DEBUG is not set\n";
		next;
	}

	if ($line =~ /CONFIG_DEBUG_GPIO=y/) {
		print "# CONFIG_DEBUG_GPIO is not set\n";
		next;
	}

	if ($line =~ /CONFIG_USB_GADGET_DEBUG_FS=y/) {
		print "# CONFIG_USB_GADGET_DEBUG_FS is not set\n";
		next;
	}

	if ($line =~ /CONFIG_TOMTOM_DEBUG=y/) {
		print "# CONFIG_TOMTOM_DEBUG is not set\n";
		next;
	}

	if ($line =~ /^# CONFIG_MODULE_HASHES is not set/) {
		print "CONFIG_MODULE_HASHES=y\n";
		next;
	}
	
	if ($line =~ /# Kernel hacking/) {
		print "$line";
#		print "Skiping section Kernel hacking\n";
		$skip_section = 1;
		print "#\n";

		print "CONFIG_PRINTK_TIME=y\n";
		print "CONFIG_ENABLE_WARN_DEPRECATED=y\n";
		print "CONFIG_ENABLE_MUST_CHECK=y\n";
		print "CONFIG_FRAME_WARN=1024\n";
		print "CONFIG_MAGIC_SYSRQ=y\n";
		print "# CONFIG_STRIP_ASM_SYMS is not set\n";
		print "# CONFIG_UNUSED_SYMBOLS is not set\n";
		print "# CONFIG_DEBUG_FS is not set\n";
		print "# CONFIG_HEADERS_CHECK is not set\n";
		print "CONFIG_DEBUG_KERNEL=y\n";
		print "# CONFIG_DEBUG_SHIRQ is not set\n";
		print "# CONFIG_DETECT_SOFTLOCKUP is not set\n";
		print "# CONFIG_BOOTPARAM_SOFTLOCKUP_PANIC is not set\n";
		print "CONFIG_DETECT_HUNG_TASK=y\n";
		print "# CONFIG_SCHED_DEBUG is not set\n";
		print "# CONFIG_SCHEDSTATS is not set\n";
		print "# CONFIG_TIMER_STATS is not set\n";
		print "# CONFIG_DEBUG_OBJECTS is not set\n";
		print "# CONFIG_DEBUG_KMEMLEAK is not set\n";
		print "# CONFIG_DEBUG_PREEMPT is not set\n";
		print "# CONFIG_DEBUG_RT_MUTEXES is not set\n";
		print "# CONFIG_RT_MUTEX_TESTER is not set\n";
		print "# CONFIG_DEBUG_SPINLOCK is not set\n";
		print "# CONFIG_DEBUG_MUTEXES is not set\n";
		print "# CONFIG_DEBUG_LOCK_ALLOC is not set\n";
		print "# CONFIG_PROVE_LOCKING is not set\n";
		print "# CONFIG_LOCK_STAT is not set\n";
		print "# CONFIG_DEBUG_SPINLOCK_SLEEP is not set\n";
		print "# CONFIG_DEBUG_LOCKING_API_SELFTESTS is not set\n";
		print "# CONFIG_DEBUG_KOBJECT is not set\n";
		print "# CONFIG_DEBUG_BUGVERBOSE is not set\n";
		print "CONFIG_DEBUG_INFO=y\n";
		print "# CONFIG_DEBUG_VM is not set\n";
		print "# CONFIG_DEBUG_WRITECOUNT is not set\n";
		print "# CONFIG_DEBUG_MEMORY_INIT is not set\n";
		print "# CONFIG_DEBUG_LIST is not set\n";
		print "# CONFIG_DEBUG_SG is not set\n";
		print "# CONFIG_DEBUG_NOTIFIERS is not set\n";
		print "# CONFIG_DEBUG_CREDENTIALS is not set\n";
		print "# CONFIG_BOOT_PRINTK_DELAY is not set\n";
		print "# CONFIG_RCU_TORTURE_TEST is not set\n";
		print "# CONFIG_RCU_CPU_STALL_DETECTOR is not set\n";
		print "# CONFIG_BACKTRACE_SELF_TEST is not set\n";
		print "# CONFIG_DEBUG_BLOCK_EXT_DEVT is not set\n";
		print "# CONFIG_DEBUG_FORCE_WEAK_PER_CPU is not set\n";
		print "# CONFIG_FAULT_INJECTION is not set\n";
		print "# CONFIG_LATENCYTOP is not set\n";
		print "# CONFIG_PAGE_POISONING is not set\n";
		print "# CONFIG_KGDB is not set\n";
		print "CONFIG_HAVE_FUNCTION_TRACER=y\n";
		print "# CONFIG_TRACING_SUPPORT is not set\n";
		print "# CONFIG_FTRACE is not set\n";
		print "# CONFIG_SAMPLES is not set\n";
		print "CONFIG_HAVE_ARCH_KGDB=y\n";
		print "CONFIG_ARM_UNWIND=y\n";
		print "# CONFIG_DEBUG_USER is not set\n";
		print "# CONFIG_DEBUG_ERRORS is not set\n";
		print "# CONFIG_DEBUG_STACK_USAGE is not set\n";
		print "# CONFIG_DEBUG_LL is not set\n";
		print "# CONFIG_EARLY_PRINTK is not set\n";
		print "\n";
	
		next;
	}

	print $line;
}

sub s5p6450_filter
{
	my $line = $_[0];

	if ($line =~ /CONFIG_TOMTOM_DEBUG=y/) {
		print "# CONFIG_TOMTOM_DEBUG is not set\n";
		next;
	}
	if ($line =~ /CONFIG_BLK_DEV_SIGNEDLOOP_RW=y/) {
		print "# CONFIG_BLK_DEV_SIGNEDLOOP_RW is not set\n";
		next;
	}

	print $line;
}

sub default_filter
{
	print $_[0];
}

if ( $ARGV[0] eq "strasbourg" ) {
	while (<STDIN>) {
		strasbourg_filter($_);
	}
} elsif ( $ARGV[0] eq "valdez" || $ARGV[0] eq "taranto" ) {
	while (<STDIN>) {
		s5p6450_filter($_);
	}
} else {
	while (<STDIN>) {
		default_filter($_);
	}
}

