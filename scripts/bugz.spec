Name:           bugz
Summary:        A command line interface to Bugzilla
Group:          Applications/Internet
Version:        1.0.0
Release:        0.1%{?dist}
License:        GPL-2
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-build
URL:            https://github.com/chuckliu1979/bugz
Source:         bugz-%{version}.tar.gz

#
Requires: file
Requires: json-c >= 0.11
BuildRequires: json-c-devel >= 0.11

%if 0%{?rhel} >= 6 || 0%{?oraclelinux} >= 6 || 0%{?fedora} >= 12
Requires: libcurl
BuildRequires: file-devel
BuildRequires: libcurl-devel
%else
Requires: curl
BuildRequires: file
BuildRequires: curl-devel
%endif

%description
Bugz is a command line interface to Bugzilla.

It was conceived as a tool to speed up the workflow for Gentoo Linux developersand contributors when dealing with bugs using Bugzilla. By avoiding the clunky web interface, the user quickly search, isolate and contribute to the project very quickly. 
Developers alike can easily extract attachments and close bugs all from the comfort of the command line.

Bugz is written in C and written in a way to be extended easily for use on other Bugzillas. 

%prep
%setup

%build
%configure

%install
rm -rf %{buildroot}
make install DESTDIR=%{buildroot}

%clean
rm -rf %{buildroot}

%files
%defattr(-,root,root) 
%doc
/usr/share/doc/bugz/*
/usr/share/man/man1/*
%{_bindir}/*

%changelog
* Sun Oct 06 2018 Chuck Liu <19246678@qq.com> - 1.0.0-0.2
- add support for searching for addresses in CC field

* Thu Mar 10 2016 Chuck Liu <19246678@qq.com> - 1.0.0-0.1
- Initial packaging

