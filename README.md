MWDMDriver
==========

A Virtual WDM Driver for Windows

This does the following:

1: Loads and Unloads using an INF.

2: timer to generate events at 1 sec duration. We capture the interrupt and implement a DPC routine to update the application counter.
Goal: To understand kernel timer functions, timer expiry event handling, semaphore handling and DPC.
We create a kernel timer using KeInitializeTimer, set the timer with KeSetTimerEx and use a period of 1s. Also we create a thread in the driver,and call KeWaitForSingleObject to wait on the timer and upon return, call our function.

3: Implement IOCTL routines to Get and Set the duration of the timer interrupt. We use one of the four different types of IOCTLs (METHOD_IN_DIRECT, METHOD_OUT_DIRECT, METHOD_NEITHER, and METHOD_BUFFERED) that are supported.
Goal: This provides an understanding of how the user does input and output buffer handling by the I/O subsystem and the driver in each case.
Hint: Buffer handling is specified in the DeviceControl function.

4: Access PCI configuration address space and print out the various PCI devices configured on the system.
Goal: To read physical memory mapped address on the system and buffer management using IOCTL calls.
Hint: Microsoft provides system support for accessing the configuration space of PCI devices by two methods:
The BUS_INTERFACE_STANDARD bus interface 
The configuration I/O request packets (IRPs), IRP_MN_READ_CONFIG and IRP_MN_WRITE_CONFIG 
