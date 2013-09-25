<strong>NOTE</strong>: NVM Primitives are offered as published interfaces.  Source code implementations of NVM Primitives are vendor-specific.  See http://www.t10.org/cgi-bin/ac.pl?t=d&f=13-064r1.pdf for the Atomic Writes submission to the INCITS T10 SCSI Standards working group.

# WELCOME TO NVM PRIMITIVES

<ol>
	<li> OVERVIEW </li>
	<li> GETTING STARTED WITH NVM PRIMITIVES </li>
	<li> IOMEMORY AND VSL INSTALLATION </li>
	<li> VSL CONFIGURATION </li>
	<li> NVM PRIMITIVES INSTALLATION </li>
	<li> NVM PRIMITIVES API REFERENCE </li>
	<li> NVM PRIMITIVES SAMPLE CODE </li>
</ol>

## 1. OVERVIEW

The NVM Primitives library, as described in the API specification, is a lightweight user space library that provides basic operations such as simple atomic writes, vectored atomic writes, and mixed vectors of atomic writes and trims. The library leverages highly-optimized primitives such as sparse addressing, atomic-writes, Persistent TRIM, etc., provided by the underlying Flash Translation Layer (FTL). The strong consistency guarantees of these Primitives allow calls within this library to achieve high performance combined with ACID compliance.  The majority of the APIs in this library offer simple, powerful interfaces to prevent torn/incomplete writes without complex journaling or double-buffering approaches. Additional APIs include the ability to query device capabilities, physical capacity, and capacity used, as well as a few other miscellaneous interfaces.

## 2. GETTING STARTED WITH NVM PRIMITIVES

See the 'DIY On-Premises -> User-space NVM Primitives' section under http://opennvm.github.io/get-started.html

## 3. IOMEMORY AND VSL INSTALLATION*

\* _For ioMemory and VSL installation and configuration assistance, please contact Fusion-io support at https://support.fusionio.com_

* NOTE: With the release of VSL 3.3.3, NVM Primitives are only supported on Fusion-branded ioMemory devices. Future VSL releases will provide support for NVM Primitives on OEM-branded ioMemory devices.

* Request download access to VSL 3.3.3, Limited Availability release and NVM Primitives library by sending email to opennvm@fusionio.com

* Download VSL 3.3.3 and the NVM Primitives from https://support.fusionio.com.

* Install a supported Linux OS and kernel (CentOS 6, RHEL 6, SLES11SP2, or Ubuntu 12.04 with a 2.6.32 kernel).

* Install Fusion ioDrive2 or ioScale device according to the _ioMemory Hardware Installation Guide for VSL 3.3.x_, downloaded with VSL 3.3.3.

* Install VSL 3.3.3, according to the _Software Installation_ section of the accompanying ioMemory VSL 3.3.3 User Guide for Linux.

* Ensure the ioMemory firmware is up-to-date by following instructions in the _Upgrading the Firmware_ section of the _ioMemory VSL 3.3.3, User Guide for Linux_

## 4. VSL CONFIGURATION

1. Login as root

2. Disable _auto_attach_ by issuing the following command:
         
         #insmod iomemory-vsl auto_attach=0
         
3. Detach the device if it is already attached:

         #fio-detach /dev/fctx
         
         (where fctx is fct1 or fct2, etc.)
         
4. Format the device with ioMemory SDK options enabled (-A for atomic writes, -P for persistent TRIM, and -e for sparse address).

         #fio-format -fyAP -e /dev/fctx
         
5. Attach the device for I/O operations.

         #fio-attach /dev/fctx
         
   This should result in device (fiox) appearing under /dev.
    
## 5. NVM PRIMITIVES INSTALLATION

1. Install the ioMemory SDK packages from https://support.fusionio.com (Under _Downloads_, select _ioMemory SDK_ under _Pick Product Options_. Look for libnvm-dev-&lt;kernel version>.&lt;version>-1.0.el6.x86_64.rpm under SDK section).

2. Install packages with the rpm utility:

         rpm -ivh libnvm-dev-<kernel version>.<version>-1.0.el6.x86_64.rpm
         
   Note: If libnvm-dev from a previous early-access release is already installed, please uninstall it before installing the packages for 0.7
   
         rpm -e <ioMemory SDK package name>
         
3. Verify that the libnvm-dev package has been installed.

         rpm -qa | grep libnvm
         libnvm-dev-<kernel version>.<version>-1.0.el6.x86_64
         
4. Verify that the following artifacts are installed:

   Header files:
   
         nvm_primitives.h, nvm_error.h and nvm_types.h should be available at /usr/include/nvm/ and knvm_primitives.h,
         nvm_types.h should be available at /usr/src/nvm/include/.
   
   Shared libraries:
   
         libnvm-primitives.so should be available at /usr/lib/nvm
   
   Kernel primitives kernel module:
   
         libknvm.ko should be available at /lib/modules/<kernel version>/kernel/lib/
   
   Sample Code:
   
         sample code should be available at /usr/share/doc/nvm/primitives/.
   
5. Note that the ioMemory SDK requires special formatting of the ioMemory device. Attaching an ioMemory device can fail if the device is not properly formatted. See the _VSL Configuration_ section above.

## 6. NVM PRIMITIVES API REFERENCE

http://opennvm.github.io/nvm-primitives-documents/

## 7. NVM PRIMITIVES SAMPLE CODE

https://github.com/opennvm/nvm-primitives/tree/master/docs/examples/user

