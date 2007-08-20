Summary:        cairo-rendered on-screen clock 
Name:           cairo-clock
Version:        0.3.3
Release:        1
URL:            http://macslow.thepimp.net/projects/cairo-clock
Source0:        cairo-clock-0.3.3.tar.gz
License:        GPL
Group:          User Interface/Desktops
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
BuildRequires:  gtk2-devel >= 2.10.0
BuildRequires:  pango-devel >= 1.2.0
BuildRequires:  fontconfig-devel
BuildRequires:  libglade2-devel >= 2.6.0
BuildRequires:  libtool autoconf
BuildRequires:  automake >= 1.9.6
BuildRequires:  librsvg2-devel >= 2.14.0
BuildRequires:  perl-XML-Parser >= 2.34

%description
It is an analog clock displaying the system-time. It leverages the new visual
features offered by Xorg 7.0 in combination with a compositing-manager (e.g.
like xcompmgr or compiz), gtk+ 2.10.0, cairo 1.2.0, libglade 2.6.0 and librsvg
2.14.0 to produce a time display with pretty-pixels.

For more up-to-date info, screenshots and screencasts take a look at the
project-homepage located at:

	http://macslow.thepimp.net/cairo-clock

%prep
%setup -q

%build
export LIBS="-lXext -lX11"
%configure
make

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall
/bin/rm -f $RPM_BUILD_ROOT%{_libdir}/*.la

%clean
rm -rf "$RPM_BUILD_ROOT"

%post
/sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%defattr(-,root,root)
%doc AUTHORS BUGS COPYING INSTALL NEWS README TODO
%{_bindir}/*
%{_datadir}/*

