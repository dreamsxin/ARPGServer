#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux-x86
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/MyleftServer.o \
	${OBJECTDIR}/authreg.o \
	${OBJECTDIR}/db_mysql.o \
	${OBJECTDIR}/ezxml.o \
	${OBJECTDIR}/function.o \
	${OBJECTDIR}/hash.o \
	${OBJECTDIR}/log.o \
	${OBJECTDIR}/luafunction.o \
	${OBJECTDIR}/message.o


# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-lm -lpthread -lmysqlclient -lcrypt -lcrypto -llua5.2 -lz

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/myleftserver

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/myleftserver: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.c} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/myleftserver ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/MyleftServer.o: MyleftServer.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -I. -I/usr/include/lua5.2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/MyleftServer.o MyleftServer.c

${OBJECTDIR}/authreg.o: authreg.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -I. -I/usr/include/lua5.2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/authreg.o authreg.c

${OBJECTDIR}/db_mysql.o: db_mysql.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -I. -I/usr/include/lua5.2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/db_mysql.o db_mysql.c

${OBJECTDIR}/ezxml.o: ezxml.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -I. -I/usr/include/lua5.2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/ezxml.o ezxml.c

${OBJECTDIR}/function.o: function.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -I. -I/usr/include/lua5.2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/function.o function.c

${OBJECTDIR}/hash.o: hash.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -I. -I/usr/include/lua5.2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/hash.o hash.c

${OBJECTDIR}/log.o: log.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -I. -I/usr/include/lua5.2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/log.o log.c

${OBJECTDIR}/luafunction.o: luafunction.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -I. -I/usr/include/lua5.2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/luafunction.o luafunction.c

${OBJECTDIR}/message.o: message.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} $@.d
	$(COMPILE.c) -O2 -I. -I/usr/include/lua5.2 -MMD -MP -MF $@.d -o ${OBJECTDIR}/message.o message.c

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/myleftserver

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
