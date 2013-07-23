<strong>NOTE</strong>: NVM Primitives are offered as published interfaces.  Source code implementations of NVM Primitives are vendor-specific.  See http://www.t10.org/cgi-bin/ac.pl?t=d&f=13-064r1.pdf for the Atomic Writes submission to the INCITS T10 SCSI Standards working group.

# WELCOME TO NVM PRIMITIVES

<ol>
	<li> OVERVIEW </li>
	<li> GETTING STARTED WITH NVM PRIMITIVES </li>
	<li> NVM PRIMITIVES API REFERENCE </li>
	<li> NVM PRIMITIVES SAMPLE CODE </li>
</ol>

## 1. OVERVIEW

The NVM Primitives library, as described in the API specification, is a lightweight user space library that provides basic operations such as simple atomic writes, vectored atomic writes, and mixed vectors of atomic writes and trim. The library leverages highly-optimized primitives such as sparse addressing, atomic-writes, Persistent TRIM, etc., provided by the underlying Flash Translation Layer (FTL). The strong consistency guarantees of these Primitives allow calls within this library to achieve high performance combined with ACID compliance.  The majority of the APIs in this library offer simple, powerful interfaces to prevent torn/incomplete writes without complex journaling or double-buffering approaches. Additional APIs include the ability to query device capabilities, physical capacity, and capacity used, as well as a few other miscellaneous interfaces.

## 2. GETTING STARTED WITH NVM PRIMITIVES

See the 'DIY On-Premises -> User-space NVM Primitives' section under http://opennvm.github.io/get-started.html

## 3. NVM PRIMITIVES API REFERENCE

http://opennvm.github.io/nvm-primitives-documents/Default.htm

## 4. NVM PRIMITIVES SAMPLE CODE

https://github.com/opennvm/nvm-primitives/tree/master/docs/examples/user