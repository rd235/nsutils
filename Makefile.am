bin_PROGRAMS = nshold nslist nsrelease netnsjoin

nshold_SOURCES = nshold.c catargv.c prefix.c

nshold_LDADD = -lbsd -lcap

nslist_SOURCES = nslist.c catargv.c memogetusername.c prefix.c printflen.c

nsrelease_SOURCES = nsrelease.c nssearch.c catargv.c prefix.c

netnsjoin_SOURCES = netnsjoin.c nssearch.c catargv.c prefix.c

netnsjoin_LDADD = -lcap

man_MANS = netnsjoin.1 nshold.1 nslist.1 nsrelease.1
 
install-exec-hook:
	ln -sf nshold $(DESTDIR)$(bindir)/cgroupnshold
	ln -sf nslist $(DESTDIR)$(bindir)/cgroupnslist
	ln -sf nsrelease $(DESTDIR)$(bindir)/cgroupnsrelease
	ln -sf nshold $(DESTDIR)$(bindir)/ipcnshold
	ln -sf nslist $(DESTDIR)$(bindir)/ipcnslist
	ln -sf nsrelease $(DESTDIR)$(bindir)/ipcnsrelease
	ln -sf nshold $(DESTDIR)$(bindir)/mntnshold
	ln -sf nslist $(DESTDIR)$(bindir)/mntnslist
	ln -sf nsrelease $(DESTDIR)$(bindir)/mntnsrelease
	ln -sf nshold $(DESTDIR)$(bindir)/netnshold
	ln -sf nslist $(DESTDIR)$(bindir)/netnslist
	ln -sf nsrelease $(DESTDIR)$(bindir)/netnsrelease
	ln -sf nshold $(DESTDIR)$(bindir)/pidnshold
	ln -sf nslist $(DESTDIR)$(bindir)/pidnslist
	ln -sf nsrelease $(DESTDIR)$(bindir)/pidnsrelease
	ln -sf nshold $(DESTDIR)$(bindir)/usernshold
	ln -sf nslist $(DESTDIR)$(bindir)/usernslist
	ln -sf nsrelease $(DESTDIR)$(bindir)/usernsrelease
	ln -sf nshold $(DESTDIR)$(bindir)/utsnshold
	ln -sf nslist $(DESTDIR)$(bindir)/utsnslist
	ln -sf nsrelease $(DESTDIR)$(bindir)/utsnsrelease
	setcap cap_sys_ptrace,cap_sys_admin+p $(DESTDIR)$(bindir)/netnsjoin

install-data-hook:
	ln -sf nshold.1 $(DESTDIR)$(man1dir)/cgroupnshold.1
	ln -sf nslist.1 $(DESTDIR)$(man1dir)/cgroupnslist.1
	ln -sf nsrelease.1 $(DESTDIR)$(man1dir)/cgroupnsrelease.1
	ln -sf nshold.1 $(DESTDIR)$(man1dir)/ipcnshold.1
	ln -sf nslist.1 $(DESTDIR)$(man1dir)/ipcnslist.1
	ln -sf nsrelease.1 $(DESTDIR)$(man1dir)/ipcnsrelease.1
	ln -sf nshold.1 $(DESTDIR)$(man1dir)/mntnshold.1
	ln -sf nslist.1 $(DESTDIR)$(man1dir)/mntnslist.1
	ln -sf nsrelease.1 $(DESTDIR)$(man1dir)/mntnsrelease.1
	ln -sf nshold.1 $(DESTDIR)$(man1dir)/netnshold.1
	ln -sf nslist.1 $(DESTDIR)$(man1dir)/netnslist.1
	ln -sf nsrelease.1 $(DESTDIR)$(man1dir)/netnsrelease.1
	ln -sf nshold.1 $(DESTDIR)$(man1dir)/pidnshold.1
	ln -sf nslist.1 $(DESTDIR)$(man1dir)/pidnslist.1
	ln -sf nsrelease.1 $(DESTDIR)$(man1dir)/pidnsrelease.1
	ln -sf nshold.1 $(DESTDIR)$(man1dir)/usernshold.1
	ln -sf nslist.1 $(DESTDIR)$(man1dir)/usernslist.1
	ln -sf nsrelease.1 $(DESTDIR)$(man1dir)/usernsrelease.1
	ln -sf nshold.1 $(DESTDIR)$(man1dir)/utsnshold.1
	ln -sf nslist.1 $(DESTDIR)$(man1dir)/utsnslist.1
	ln -sf nsrelease.1 $(DESTDIR)$(man1dir)/utsnsrelease.1

uninstall-hook:
	rm -f $(DESTDIR)$(bindir)/cgroupnshold
	rm -f $(DESTDIR)$(bindir)/cgroupnslist
	rm -f $(DESTDIR)$(bindir)/cgroupnsrelease
	rm -f $(DESTDIR)$(bindir)/ipcnshold
	rm -f $(DESTDIR)$(bindir)/ipcnslist
	rm -f $(DESTDIR)$(bindir)/ipcnsrelease
	rm -f $(DESTDIR)$(bindir)/mntnshold
	rm -f $(DESTDIR)$(bindir)/mntnslist
	rm -f $(DESTDIR)$(bindir)/mntnsrelease
	rm -f $(DESTDIR)$(bindir)/netnshold
	rm -f $(DESTDIR)$(bindir)/netnslist
	rm -f $(DESTDIR)$(bindir)/netnsrelease
	rm -f $(DESTDIR)$(bindir)/pidnshold
	rm -f $(DESTDIR)$(bindir)/pidnslist
	rm -f $(DESTDIR)$(bindir)/pidnsrelease
	rm -f $(DESTDIR)$(bindir)/usernshold
	rm -f $(DESTDIR)$(bindir)/usernslist
	rm -f $(DESTDIR)$(bindir)/usernsrelease
	rm -f $(DESTDIR)$(bindir)/utsnshold
	rm -f $(DESTDIR)$(bindir)/utsnslist
	rm -f $(DESTDIR)$(bindir)/utsnsrelease
	rm -f $(DESTDIR)$(man1dir)/cgroupnshold.1
	rm -f $(DESTDIR)$(man1dir)/cgroupnslist.1
	rm -f $(DESTDIR)$(man1dir)/cgroupnsrelease.1
	rm -f $(DESTDIR)$(man1dir)/ipcnshold.1
	rm -f $(DESTDIR)$(man1dir)/ipcnslist.1
	rm -f $(DESTDIR)$(man1dir)/ipcnsrelease.1
	rm -f $(DESTDIR)$(man1dir)/mntnshold.1
	rm -f $(DESTDIR)$(man1dir)/mntnslist.1
	rm -f $(DESTDIR)$(man1dir)/mntnsrelease.1
	rm -f $(DESTDIR)$(man1dir)/netnshold.1
	rm -f $(DESTDIR)$(man1dir)/netnslist.1
	rm -f $(DESTDIR)$(man1dir)/netnsrelease.1
	rm -f $(DESTDIR)$(man1dir)/pidnshold.1
	rm -f $(DESTDIR)$(man1dir)/pidnslist.1
	rm -f $(DESTDIR)$(man1dir)/pidnsrelease.1
	rm -f $(DESTDIR)$(man1dir)/usernshold.1
	rm -f $(DESTDIR)$(man1dir)/usernslist.1
	rm -f $(DESTDIR)$(man1dir)/usernsrelease.1
	rm -f $(DESTDIR)$(man1dir)/utsnshold.1
	rm -f $(DESTDIR)$(man1dir)/utsnslist.1
	rm -f $(DESTDIR)$(man1dir)/utsnsrelease.1
