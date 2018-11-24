# This is the spec file for building a RPM of dirt-client
# vim: expandtab
# Originaly created by Rodrigo Parra Novo

Summary: Dirt:  A MUD client for Unix
Summary(pt_BR): Dirt: Um cliente de MUD para Unix
Name:           dirt-client
Version:        0.60.00
Release:        2
License:        GPLv2
Group:          Amusements/Games
Group(pt_BR):   Passatempos/Jogos
Packager:       Billy Holmes <billy@gonoph.net>
URL:            https://github.com/gonoph/%{name}
Source0:        https://github.com/gonoph/%{name}/archive/%{name}-%{version}-%{release}.tar.gz

Requires:       libz ncurses
Requires:       python(abi) = 2.7
Requires:       perl >= 5.16

BuildRequires:  perl-devel libz-devel python-devel gcc-c++
BuildRequires:  perl(ExtUtils-Embed)
BuildRequires:  python(abi) = 2.7
BuildRequires:  perl >= 5.16

Provides:       mudclient

%description
dirt is a MUD client running under Unix. Under Linux console, it uses direct
Virtual Console access for high speed, but it can also run in an xterm or on a
VT100/ANSI compatible terminal. Embedded Perl and Python provide a high degree
of tweakability.

%description -l pt_BR
O dirt é um cliente de MUD que roda no Unix. No console virtual do Linux
ele usa acesso direto ao console para obter uma velocidade  maior,  mas
ele também pode rodar em um xterm, ou em um terminal  compatível  com a
especificação VT100/ANSI. O dirt tem suporte a scripts via plugins, sendo que
já estão disponíveis perl e python.

%prep
%autosetup

%build
export CFLAGS="%{optflags} -fPIC"
export CXXFLAGS="%{optflags} -fPIC"
export LDFLAGS="-s"
./configure --prefix=%{_prefix} --with-python --with-perl
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
%make_install

%files
%defattr (-, root, root)
%doc README.md README.1ST INSTALL doc/
%{prefix}/bin/dirt
%{prefix}/lib/dirt

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
Wed Nov 14 19:43:53 UTC 2018
* Wed Nov 14 2018 Billy Holmes <billy@gonoph.net>
- Updated spec file to be more consistent with GNU standards and updated RPM guides

* Fri Jan 26 2000 Bob McElrath <mcelrath+dirt@draal.physics.wisc.edu>
- Renamed everything to "dirt" from mcl.

* Sun Oct 03 1999 Rodrigo Parra Novo <rodarvus@conectiva.com.br>
- Modified the spec file to work with autoconf
- Allowed non-root builds
- Allowed the use of prefixes in the install
- Create mcl.spec directly from mcl.spec.in
- Modified the RPM group mcl belongs to

* Mon Sep 24 1999 Erwin S. Andreasen <erwin@andreasen.org>
- Moved samples to /usr/lib/mcl/, tweaked things.

* Mon Sep 20 1999 Erwin S. Andreasen <erwin@andreasen.org>
- Plugins added

* Fri Mar 12 1999 Erwin S. Andreasen <erwin@andreasen.org>
- Trying to build the 0.50.02 version myself :)

* Tue Feb 16 1999 Rodrigo Parra Novo <rodarvus@conectiva.com.br>
- Updated package to version 0.50.01
- Changed version of perl needed to 5.004
- Removed patch to make mcl compile on an alpha

* Mon Feb 15 1999 Rodrigo Parra Novo <rodarvus@conectiva.com.br>
- Updated package to version 0.50.00
- Added requires perl >= 5.00502
- Added patch to make alpha compile again
- Now mcl goes stripped by default

* Wed Dec 30 1998 Rodrigo Parra Novo <rodarvus@conectiva.com.br>
- First build with RPM (version 0.42.05)
- Added pt_BR translations
