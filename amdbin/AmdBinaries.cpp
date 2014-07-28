/*
 *  CLRadeonExtender - Unofficial OpenCL Radeon Extensions Library
 *  Copyright (C) 2014 Mateusz Szpakowski
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <CLRX/Config.h>
#include <cstdlib>
#include <cstring>
#include <elf.h>
#include <algorithm>
#include <climits>
#include <unordered_map>
#include <map>
#include <utility>
#include <string>
#include <vector>
#include <CLRX/Utilities.h>
#include <CLRX/AmdBinaries.h>

using namespace CLRX;

static const KernelArgType x86ArgTypeTable[]
{
    KernelArgType::VOID,
    KernelArgType::CHAR,
    KernelArgType::SHORT,
    KernelArgType::INT,
    KernelArgType::LONG,
    KernelArgType::FLOAT,
    KernelArgType::DOUBLE,
    KernelArgType::POINTER,
    KernelArgType::CHAR2,
    KernelArgType::CHAR3,
    KernelArgType::CHAR4,
    KernelArgType::CHAR8,
    KernelArgType::CHAR16,
    KernelArgType::SHORT2,
    KernelArgType::SHORT3,
    KernelArgType::SHORT4,
    KernelArgType::SHORT8,
    KernelArgType::SHORT16,
    KernelArgType::INT2,
    KernelArgType::INT3,
    KernelArgType::INT4,
    KernelArgType::INT8,
    KernelArgType::INT16,
    KernelArgType::LONG2,
    KernelArgType::LONG3,
    KernelArgType::LONG4,
    KernelArgType::LONG8,
    KernelArgType::LONG16,
    KernelArgType::FLOAT2,
    KernelArgType::FLOAT3,
    KernelArgType::FLOAT4,
    KernelArgType::FLOAT8,
    KernelArgType::FLOAT16,
    KernelArgType::DOUBLE2,
    KernelArgType::DOUBLE3,
    KernelArgType::DOUBLE4,
    KernelArgType::DOUBLE8,
    KernelArgType::DOUBLE16,
    KernelArgType::SAMPLER
};

static const KernelArgType gpuArgTypeTable[]
{
    KernelArgType::UCHAR,
    KernelArgType::UCHAR2,
    KernelArgType::UCHAR3,
    KernelArgType::UCHAR4,
    KernelArgType::UCHAR8,
    KernelArgType::UCHAR16,
    KernelArgType::CHAR,
    KernelArgType::CHAR2,
    KernelArgType::CHAR3,
    KernelArgType::CHAR4,
    KernelArgType::CHAR8,
    KernelArgType::CHAR16,
    KernelArgType::USHORT,
    KernelArgType::USHORT2,
    KernelArgType::USHORT3,
    KernelArgType::USHORT4,
    KernelArgType::USHORT8,
    KernelArgType::USHORT16,
    KernelArgType::SHORT,
    KernelArgType::SHORT2,
    KernelArgType::SHORT3,
    KernelArgType::SHORT4,
    KernelArgType::SHORT8,
    KernelArgType::SHORT16,
    KernelArgType::UINT,
    KernelArgType::UINT2,
    KernelArgType::UINT3,
    KernelArgType::UINT4,
    KernelArgType::UINT8,
    KernelArgType::UINT16,
    KernelArgType::INT,
    KernelArgType::INT2,
    KernelArgType::INT3,
    KernelArgType::INT4,
    KernelArgType::INT8,
    KernelArgType::INT16,
    KernelArgType::ULONG,
    KernelArgType::ULONG2,
    KernelArgType::ULONG3,
    KernelArgType::ULONG4,
    KernelArgType::ULONG8,
    KernelArgType::ULONG16,
    KernelArgType::LONG,
    KernelArgType::LONG2,
    KernelArgType::LONG3,
    KernelArgType::LONG4,
    KernelArgType::LONG8,
    KernelArgType::LONG16,
    KernelArgType::FLOAT,
    KernelArgType::FLOAT2,
    KernelArgType::FLOAT3,
    KernelArgType::FLOAT4,
    KernelArgType::FLOAT8,
    KernelArgType::FLOAT16,
    KernelArgType::DOUBLE,
    KernelArgType::DOUBLE2,
    KernelArgType::DOUBLE3,
    KernelArgType::DOUBLE4,
    KernelArgType::DOUBLE8,
    KernelArgType::DOUBLE16
};


static const uint32_t elfMagicValue = 0x464c457fU;

/* kernel info class */

KernelInfo::KernelInfo() : argsNum(0), argInfos(nullptr)
{ }

KernelInfo::~KernelInfo()
{
    delete[] argInfos;
}

KernelInfo::KernelInfo(const KernelInfo& cp)
{
    kernelName = cp.kernelName;
    argsNum = cp.argsNum;
    argInfos = new KernelArg[argsNum];
    std::copy(cp.argInfos, cp.argInfos + argsNum, argInfos);
}

KernelInfo::KernelInfo(KernelInfo&& cp)
{
    kernelName = std::move(cp.kernelName);
    argsNum = cp.argsNum;
    argInfos = cp.argInfos;
    cp.argsNum = 0;
    cp.argInfos = nullptr; // reset pointer
}

KernelInfo& KernelInfo::operator=(const KernelInfo& cp)
{
    delete[] argInfos;
    argInfos = nullptr;
    kernelName = cp.kernelName;
    argsNum = cp.argsNum;
    argInfos = new KernelArg[argsNum];
    std::copy(cp.argInfos, cp.argInfos + argsNum, argInfos);
    return *this;
}

KernelInfo& KernelInfo::operator=(KernelInfo&& cp)
{
    delete[] argInfos;
    argInfos = nullptr;
    kernelName = std::move(cp.kernelName);
    argsNum = cp.argsNum;
    argInfos = cp.argInfos;
    cp.argsNum = 0;
    cp.argInfos = nullptr; // reset pointer
    return *this;
}

void KernelInfo::allocateArgs(cxuint argsNum)
{
    delete[] argInfos;
    argInfos = nullptr;
    this->argsNum = argsNum;
    argInfos = new KernelArg[argsNum];
}

void KernelInfo::reallocateArgs(cxuint newArgsNum)
{
    if (newArgsNum == argsNum)
        return; // do nothing
    KernelArg* newArgInfos = new KernelArg[newArgsNum];
    if (argInfos != nullptr)
        std::copy(argInfos, argInfos + std::min(newArgsNum, argsNum), newArgInfos);
    delete[] argInfos;
    argInfos = newArgInfos;
    argsNum = newArgsNum;
}

/* determine unfinished strings region in string table for checking further consistency */
static size_t unfinishedRegionOfStringTable(const char* table, size_t size)
{
    if (size == 0) // if zero
        return 0;
    size_t k;
    for (k = size-1; k>0 && table[k]!=0; k--);
    
    return (table[k]==0)?k+1:k;
}

/* elf32 types */

const uint8_t CLRX::Elf32Types::ELFCLASS = ELFCLASS32;
const uint32_t CLRX::Elf32Types::bitness = 32;
const char* CLRX::Elf32Types::bitName = "32";

/* elf64 types */

const uint8_t CLRX::Elf64Types::ELFCLASS = ELFCLASS64;
const cxuint CLRX::Elf64Types::bitness = 64;
const char* CLRX::Elf64Types::bitName = "64";

template class ElfBinaryTemplate<CLRX::Elf32Types>;
template class ElfBinaryTemplate<CLRX::Elf64Types>;

/* ElfBinaryTemplate */

template<typename Types>
ElfBinaryTemplate<Types>::ElfBinaryTemplate() : binaryCodeSize(0), binaryCode(nullptr),
        sectionStringTable(nullptr), symbolStringTable(nullptr),
        symbolTable(nullptr), dynSymStringTable(nullptr), dynSymTable(nullptr),
        symbolsNum(0), dynSymbolsNum(0),
        symbolEntSize(0), dynSymEntSize(0)
{ }

template<typename Types>
ElfBinaryTemplate<Types>::~ElfBinaryTemplate()
{ }

template<typename Types>
ElfBinaryTemplate<Types>::ElfBinaryTemplate(size_t binaryCodeSize, char* binaryCode,
             cxuint creationFlags) :
        binaryCodeSize(0), binaryCode(nullptr),
        sectionStringTable(nullptr), symbolStringTable(nullptr),
        symbolTable(nullptr), dynSymStringTable(nullptr), dynSymTable(nullptr),
        symbolsNum(0), dynSymbolsNum(0), symbolEntSize(0), dynSymEntSize(0)
{
    this->creationFlags = creationFlags;
    this->binaryCode = binaryCode;
    this->binaryCodeSize = binaryCodeSize;
    
    if (binaryCodeSize < sizeof(typename Types::Ehdr))
        throw Exception("Binary is too small!!!");
    
    const typename Types::Ehdr* ehdr =
            reinterpret_cast<const typename Types::Ehdr*>(binaryCode);
    
    if (ULEV(*reinterpret_cast<const uint32_t*>(binaryCode)) != elfMagicValue)
        throw Exception("This is not ELF binary");
    if (ehdr->e_ident[EI_CLASS] != Types::ELFCLASS)
        throw Exception(std::string("This is not ")+Types::bitName+"bit ELF binary");
    if (ehdr->e_ident[EI_DATA] != ELFDATA2LSB)
        throw Exception("Other than little-endian binaries are not supported!");
    
    if (ULEV(ehdr->e_phoff) != 0)
    {   /* reading and checking program headers */
        if (ULEV(ehdr->e_phoff) > binaryCodeSize)
            throw Exception("ProgramHeaders offset out of range!");
        if (ULEV(ehdr->e_phoff) + size_t(ULEV(ehdr->e_phentsize))*ULEV(ehdr->e_phnum) >
            binaryCodeSize)
            throw Exception("ProgramHeaders offset+size out of range!");
        
        cxuint phnum = ULEV(ehdr->e_phnum);
        for (cxuint i = 0; i < phnum; i++)
        {
            const typename Types::Phdr& phdr = getProgramHeader(i);
            if (ULEV(phdr.p_offset) > binaryCodeSize)
                throw Exception("Segment offset out of range!");
            if (ULEV(phdr.p_offset)+ULEV(phdr.p_filesz) > binaryCodeSize)
                throw Exception("Segment offset+size out of range!");
        }
    }
    
    if (ULEV(ehdr->e_shoff) != 0 && ULEV(ehdr->e_shstrndx) != SHN_UNDEF)
    {   /* indexing of sections */
        if (ULEV(ehdr->e_shoff) > binaryCodeSize)
            throw Exception("SectionHeaders offset out of range!");
        if (ULEV(ehdr->e_shoff) + size_t(ULEV(ehdr->e_shentsize))*ULEV(ehdr->e_shnum) >
            binaryCodeSize)
            throw Exception("SectionHeaders offset+size out of range!");
        if (ULEV(ehdr->e_shstrndx) >= ULEV(ehdr->e_shnum))
            throw Exception("Shstrndx out of range!");
        
        typename Types::Shdr& shstrShdr = getSectionHeader(ULEV(ehdr->e_shstrndx));
        sectionStringTable = binaryCode + ULEV(shstrShdr.sh_offset);
        const size_t unfinishedShstrPos = unfinishedRegionOfStringTable(
                    sectionStringTable, ULEV(shstrShdr.sh_size));
        
        const typename Types::Shdr* symTableHdr = nullptr;
        const typename Types::Shdr* dynSymTableHdr = nullptr;
        
        cxuint shnum = ULEV(ehdr->e_shnum);
        for (cxuint i = 0; i < shnum; i++)
        {
            const typename Types::Shdr& shdr = getSectionHeader(i);
            if (ULEV(shdr.sh_offset) > binaryCodeSize)
                throw Exception("Section offset out of range!");
            if (ULEV(shdr.sh_type) != SHT_NOBITS)
                if (ULEV(shdr.sh_offset)+ULEV(shdr.sh_size) > binaryCodeSize)
                    throw Exception("Section offset+size out of range!");
            if (ULEV(shdr.sh_link) >= ULEV(ehdr->e_shnum))
                throw Exception("Section link out of range!");
            
            const typename Types::Size sh_nameindx = ULEV(shdr.sh_name);
            if (sh_nameindx >= ULEV(shstrShdr.sh_size))
                throw Exception("Section name index out of range!");
            
            if (sh_nameindx >= unfinishedShstrPos)
                throw Exception("Unfinished section name!");
            
            const char* shname = sectionStringTable + sh_nameindx;
            
            if (*shname != 0 && (creationFlags & ELF_CREATE_SECTIONMAP) != 0)
                sectionIndexMap.insert(std::make_pair(shname, i));
            if (ULEV(shdr.sh_type) == SHT_SYMTAB)
                symTableHdr = &shdr;
            if (ULEV(shdr.sh_type) == SHT_DYNSYM)
                dynSymTableHdr = &shdr;
        }
        
        if (symTableHdr != nullptr)
        {   // indexing symbols
            if (ULEV(symTableHdr->sh_entsize) < sizeof(typename Types::Sym))
                throw Exception("SymTable entry size is too small!");
            
            symbolEntSize = ULEV(symTableHdr->sh_entsize);
            symbolTable = binaryCode + ULEV(symTableHdr->sh_offset);
            if (ULEV(symTableHdr->sh_link) == SHN_UNDEF)
                throw Exception("Symbol table doesnt have string table");
            
            typename Types::Shdr& symstrShdr = getSectionHeader(ULEV(symTableHdr->sh_link));
            symbolStringTable = binaryCode + ULEV(symstrShdr.sh_offset);
            
            const size_t unfinishedSymstrPos = unfinishedRegionOfStringTable(
                    symbolStringTable, ULEV(symstrShdr.sh_size));
            symbolsNum = ULEV(symTableHdr->sh_size)/ULEV(symTableHdr->sh_entsize);
            
            for (typename Types::Size i = 0; i < symbolsNum; i++)
            {   /* verify symbol names */
                const typename Types::Sym& sym = getSymbol(i);
                const typename Types::Size symnameindx = ULEV(sym.st_name);
                if (symnameindx >= ULEV(symstrShdr.sh_size))
                    throw Exception("Symbol name index out of range!");
                if (symnameindx >= unfinishedSymstrPos)
                    throw Exception("Unfinished symbol name!");
                
                const char* symname = symbolStringTable + symnameindx;
                // add to symbol map
                if (*symname != 0 && (creationFlags & ELF_CREATE_SYMBOLMAP) != 0)
                    symbolIndexMap.insert(std::make_pair(symname, i));
            }
        }
        if (dynSymTableHdr != nullptr)
        {   // indexing dynamic symbols
            if (ULEV(dynSymTableHdr->sh_entsize) < sizeof(typename Types::Sym))
                throw Exception("DynSymTable entry size is too small!");
            
            dynSymEntSize = ULEV(dynSymTableHdr->sh_entsize);
            dynSymTable = binaryCode + ULEV(dynSymTableHdr->sh_offset);
            if (ULEV(dynSymTableHdr->sh_link) == SHN_UNDEF)
                throw Exception("DynSymbol table doesnt have string table");
            
            typename Types::Shdr& dynSymstrShdr =
                    getSectionHeader(ULEV(dynSymTableHdr->sh_link));
            dynSymbolsNum = ULEV(dynSymTableHdr->sh_size)/ULEV(dynSymTableHdr->sh_entsize);
            
            dynSymStringTable = binaryCode + ULEV(dynSymstrShdr.sh_offset);
            const size_t unfinishedSymstrPos = unfinishedRegionOfStringTable(
                    dynSymStringTable, ULEV(dynSymstrShdr.sh_size));
            
            for (typename Types::Size i = 0; i < dynSymbolsNum; i++)
            {   /* verify symbol names */
                const typename Types::Sym& sym = getDynSymbol(i);
                const typename Types::Size symnameindx = ULEV(sym.st_name);
                if (symnameindx >= ULEV(dynSymstrShdr.sh_size))
                    throw Exception("DynSymbol name index out of range!");
                if (symnameindx >= unfinishedSymstrPos)
                    throw Exception("Unfinished dynsymbol name!");
                
                const char* symname = dynSymStringTable + symnameindx;
                // add to symbol map
                if (*symname != 0 && (creationFlags & ELF_CREATE_DYNSYMMAP) != 0)
                    dynSymIndexMap.insert(std::make_pair(symname, i));
            }
        }
    }
}

template<typename Types>
uint16_t ElfBinaryTemplate<Types>::getSectionIndex(const char* name) const
{
    if (hasSectionMap())
    {
        SectionIndexMap::const_iterator it = sectionIndexMap.find(name);
        if (it == sectionIndexMap.end())
            throw Exception(std::string("Cant find Elf")+Types::bitName+" Section");
        return it->second;
    }
    else
    {
        for (cxuint i = 0; i < getSectionHeadersNum(); i++)
        {
            if (::strcmp(getSectionName(i), name) == 0)
                return i;
        }
        throw Exception(std::string("Cant find Elf")+Types::bitName+" Section");
    }
}

template<typename Types>
ElfBinaryTemplate<Types>::SectionIndexMap::const_iterator
ElfBinaryTemplate<Types>::getSectionIter(const char* name) const
{
    SectionIndexMap::const_iterator it = sectionIndexMap.find(name);
    if (it == sectionIndexMap.end())
        throw Exception(std::string("Cant find Elf")+Types::bitName+" Section");
    return it;
}

template<typename Types>
typename Types::Size ElfBinaryTemplate<Types>::getSymbolIndex(const char* name) const
{
    SymbolIndexMap::const_iterator it = symbolIndexMap.find(name);
    if (it == symbolIndexMap.end())
        throw Exception(std::string("Cant find Elf")+Types::bitName+" Symbol");
    return it->second;
}

template<typename Types>
typename Types::Size ElfBinaryTemplate<Types>::getDynSymbolIndex(const char* name) const
{
    SymbolIndexMap::const_iterator it = dynSymIndexMap.find(name);
    if (it == dynSymIndexMap.end())
        throw Exception(std::string("Cant find Elf")+Types::bitName+" DynSymbol");
    return it->second;
}

template<typename Types>
ElfBinaryTemplate<Types>::SymbolIndexMap::const_iterator 
ElfBinaryTemplate<Types>::getSymbolIter(const char* name) const
{
    SymbolIndexMap::const_iterator it = symbolIndexMap.find(name);
    if (it == symbolIndexMap.end())
        throw Exception(std::string("Cant find Elf")+Types::bitName+" Symbol");
    return it;
}

template<typename Types>
ElfBinaryTemplate<Types>::SymbolIndexMap::const_iterator 
ElfBinaryTemplate<Types>::getDynSymbolIter(const char* name) const
{
    SymbolIndexMap::const_iterator it = dynSymIndexMap.find(name);
    if (it == dynSymIndexMap.end())
        throw Exception(std::string("Cant find Elf")+Types::bitName+" DynSymbol");
    return it;
}

/* AMD inner GPU binary */

AmdInnerGPUBinary32::AmdInnerGPUBinary32(const std::string& _kernelName,
         size_t binaryCodeSize, char* binaryCode, cxuint creationFlags)
        : ElfBinary32(binaryCodeSize, binaryCode, creationFlags), kernelName(_kernelName)
{ }

template<typename ArgSym>
static size_t skipStructureArgX86(const ArgSym* argDescTable,
            size_t argDescsNum, size_t startPos)
{
    size_t nestedLevel = 0;
    size_t pos = startPos;
    do {
        const ArgSym& argDesc = argDescTable[pos++];
        if (ULEV(argDesc.argType) == 0x28)
            nestedLevel++;
        else if (ULEV(argDesc.argType) == 0)
            nestedLevel--;
    } while (nestedLevel != 0 && pos < argDescsNum);
    if (nestedLevel != 0)
        throw Exception("Unfinished kernel argument structure");
    return pos;
}

/* AMD inner X86 binary */

struct CLRX_INTERNAL AmdInnerX86Types : Elf32Types
{
    typedef X86KernelArgSym KernelArgSym;
    typedef ElfBinary32 ElfBinary;
    static const size_t argDescsNumOffset;
    static const size_t argDescsNumESize;
    static const size_t argDescTableOffset;
};

struct CLRX_INTERNAL AmdInnerX86_64Types : Elf64Types
{
    typedef X86_64KernelArgSym KernelArgSym;
    typedef ElfBinary64 ElfBinary;
    static const size_t argDescsNumOffset;
    static const size_t argDescsNumESize;
    static const size_t argDescTableOffset;
};

const size_t AmdInnerX86Types::argDescsNumOffset = 44;
const size_t AmdInnerX86Types::argDescsNumESize = 12;
const size_t AmdInnerX86Types::argDescTableOffset = 32;

const size_t AmdInnerX86_64Types::argDescsNumOffset = 80;
const size_t AmdInnerX86_64Types::argDescsNumESize = 16;
const size_t AmdInnerX86_64Types::argDescTableOffset = 64;


template<typename Types>
static size_t getKernelInfosInternal(const typename Types::ElfBinary& elf,
             KernelInfo*& kernelInfos)
{
    delete[] kernelInfos;
    kernelInfos = nullptr;
    
    if (!elf) return 0;
    
    cxuint rodataIndex = SHN_UNDEF;
    try
    { rodataIndex = elf.getSectionIndex(".rodata"); }
    catch(const Exception& ex)
    { return 0; /* no section */ }
    
    const typename Types::Shdr& rodataHdr = elf.getSectionHeader(rodataIndex);
    
    /* get kernel metadata symbols */
    std::vector<typename Types::Size> choosenSyms;
    const size_t dynSymbolsNum = elf.getDynSymbolsNum();
    for (typename Types::Size i = 0; i < dynSymbolsNum; i++)
    {
        const char* symName = elf.getDynSymbolName(i);
        const size_t len = ::strlen(symName);
        if (len < 18 || (::strncmp(symName, "__OpencCL_", 9) != 0 &&
            ::strcmp(symName+len-9, "_metadata") != 0)) // not metdata then skip
            continue;
        choosenSyms.push_back(i);
    }
    
    try
    {
    kernelInfos = new KernelInfo[choosenSyms.size()];
    
    const char* binaryCode = elf.getBinaryCode();
    
    const size_t unfinishedRegion = unfinishedRegionOfStringTable(
        binaryCode + ULEV(rodataHdr.sh_offset), ULEV(rodataHdr.sh_size));
    
    size_t ki = 0;
    for (auto i: choosenSyms)
    {
        const typename Types::Sym& sym = elf.getDynSymbol(i);
        if (ULEV(sym.st_shndx) >= elf.getSectionHeadersNum())
            throw Exception("Metadata section index out of range");
        
        const typename Types::Shdr& dataHdr = 
                elf.getSectionHeader(ULEV(sym.st_shndx)); // from symbol
        const size_t fileOffset = ULEV(sym.st_value) - ULEV(dataHdr.sh_addr) +
                ULEV(dataHdr.sh_offset);
        
        if (fileOffset < ULEV(dataHdr.sh_offset) ||
            fileOffset >= ULEV(dataHdr.sh_offset) + ULEV(dataHdr.sh_size))
            throw Exception("File offset of kernelMetadata out of range!");
        const char* data = binaryCode + fileOffset;
        
        /* parse number of args */
        typename Types::Size argDescsNum =
                (*reinterpret_cast<const uint32_t*>(data) -
                Types::argDescsNumOffset)/Types::argDescsNumESize;
        KernelInfo& kernelInfo = kernelInfos[ki++];
        
        const char* symName = elf.getDynSymbolName(i);
        const size_t len = ::strlen(symName);
        kernelInfo.kernelName.assign(symName+9, len-18);
        kernelInfo.allocateArgs(argDescsNum>>1);
        
        /* get argument info */
        const typename Types::KernelArgSym* argDescTable =
                reinterpret_cast<const typename Types::KernelArgSym*>(
                    data + Types::argDescTableOffset);
        
        cxuint realArgsNum = 0;
        for (size_t ai = 0; ai < argDescsNum; ai++)
        {
            const typename Types::KernelArgSym& argNameSym = argDescTable[ai];
            cxuint argType = ULEV(argNameSym.argType);
            
            if (argType == 0x28) // skip structure
                ai = skipStructureArgX86<typename Types::KernelArgSym>(
                    argDescTable, argDescsNum, ai);
            else // if not structure
                ai++;
            const typename Types::KernelArgSym& argTypeSym = argDescTable[ai];
            
            KernelArg& karg = kernelInfo.argInfos[realArgsNum++];
            const size_t rodataHdrOffset = ULEV(rodataHdr.sh_offset);
            const size_t rodataHdrSize= ULEV(rodataHdr.sh_size);
            if (argNameSym.getNameOffset() < rodataHdrOffset ||
                argNameSym.getNameOffset() >= rodataHdrOffset+rodataHdrSize)
                throw Exception("kernel arg name offset out of range!");
            
            if (argNameSym.getNameOffset()-rodataHdrOffset >= unfinishedRegion)
                throw Exception("Arg name is unfinished!");
            
            if (argTypeSym.getNameOffset() < rodataHdrOffset ||
                argTypeSym.getNameOffset() >= rodataHdrOffset+rodataHdrSize)
                throw Exception("kernel arg type offset out of range!");
            
            if (argTypeSym.getNameOffset()-rodataHdrOffset >= unfinishedRegion)
                throw Exception("Type name is unfinished!");
            
            karg.argName = binaryCode + argNameSym.getNameOffset();
            
            if (argType != 0x28)
            {
                if (argType > 0x26)
                    throw Exception("Unknown kernel arg type");
                karg.argType = x86ArgTypeTable[argType];
                if (karg.argType == KernelArgType::POINTER &&
                    (ULEV(argNameSym.ptrAccess) &
                            (KARG_PTR_READ_ONLY|KARG_PTR_WRITE_ONLY)) != 0)
                    karg.argType = KernelArgType::IMAGE;
            }
            else // if structure
                karg.argType = KernelArgType::STRUCTURE;
            
            karg.ptrSpace = static_cast<KernelPtrSpace>(ULEV(argNameSym.ptrType));
            karg.ptrAccess = ULEV(argNameSym.ptrAccess);
            karg.typeName = binaryCode + argTypeSym.getNameOffset();
        }
        kernelInfo.reallocateArgs(realArgsNum);
    }
    }
    catch(...) // if exception happens
    {
        delete[] kernelInfos;
        kernelInfos = nullptr;
        throw;
    }
    return choosenSyms.size();
}

AmdInnerX86Binary32::AmdInnerX86Binary32(
            size_t binaryCodeSize, char* binaryCode, cxuint creationFlags) :
            ElfBinary32(binaryCodeSize, binaryCode, creationFlags)
{ }

uint32_t AmdInnerX86Binary32::getKernelInfos(KernelInfo*& kernelInfos) const
{
    return getKernelInfosInternal<AmdInnerX86Types>(*this, kernelInfos);
}

AmdInnerX86Binary64::AmdInnerX86Binary64(
            size_t binaryCodeSize, char* binaryCode, cxuint creationFlags) :
            ElfBinary64(binaryCodeSize, binaryCode, creationFlags)
{ }

size_t AmdInnerX86Binary64::getKernelInfos(KernelInfo*& kernelInfos) const
{
    return getKernelInfosInternal<AmdInnerX86_64Types>(*this, kernelInfos);
}

/* AmdMaiBinaryBase */

AmdMainBinaryBase::AmdMainBinaryBase(AmdMainType _type) : type(_type),
        kernelInfosNum(0), kernelInfos(nullptr)
{ }

AmdMainBinaryBase::~AmdMainBinaryBase()
{
    delete[] kernelInfos;
}

const KernelInfo& AmdMainBinaryBase::getKernelInfo(const char* name) const
{
    KernelInfoMap::const_iterator it = kernelInfosMap.find(name);
    if (it == kernelInfosMap.end())
        throw Exception("Cant find kernel name");
    return kernelInfos[it->second];
}

struct InitKernelArgMapEntry {
    uint32_t index;
    KernelArgType argType;
    KernelArgType origArgType;
    KernelPtrSpace ptrSpace;
    uint32_t ptrAccess;
    size_t namePos;
    size_t typePos;
    
    InitKernelArgMapEntry() : index(0), argType(KernelArgType::VOID),
        origArgType(KernelArgType::VOID),
        ptrSpace(KernelPtrSpace::NONE), ptrAccess(0), namePos(0), typePos(0)
    { }
};

static const cxuint vectorIdTable[17] =
{ UINT_MAX, 0, 1, 2, 3, UINT_MAX, UINT_MAX, UINT_MAX, 4,
  UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX, 5 };

static const KernelArgType determineKernelArgType(const char* typeString,
           cxuint vectorSize, size_t lineNo)
{
    KernelArgType outType;
    
    if (vectorSize > 16)
        throw ParseException(lineNo, "Wrong vector size");
    const cxuint vectorId = vectorIdTable[vectorSize];
    if (vectorId == UINT_MAX)
        throw ParseException(lineNo, "Wrong vector size");
    
    if (::strncmp(typeString, "float:", 6) == 0)
        outType = gpuArgTypeTable[8*6+vectorId];
    else if (::strncmp(typeString, "double:", 7) == 0)
        outType = gpuArgTypeTable[9*6+vectorId];
    else if ((typeString[0] == 'i' || typeString[0] == 'u'))
    {
        /* indexBase - choose between unsigned/signed */
        const cxuint indexBase = (typeString[0] == 'i')?6:0;
        if (typeString[1] == '8')
        {
            if (typeString[2] != ':')
                throw ParseException(lineNo, "Cant parse type");
            outType = gpuArgTypeTable[indexBase+vectorId];
        }
        else
        {
            if (typeString[1] == '1' && typeString[2] == '6')
                outType = gpuArgTypeTable[indexBase+2*6+vectorId];
            else if (typeString[1] == '3' && typeString[2] == '2')
                outType = gpuArgTypeTable[indexBase+4*6+vectorId];
            else if (typeString[1] == '6' && typeString[2] == '4')
                outType = gpuArgTypeTable[indexBase+6*6+vectorId];
            else // if not determined
                throw ParseException(lineNo, "Cant parse type");
            if (typeString[3] != ':')
                throw ParseException(lineNo, "Cant parse type");
        }
    }
    else
        throw ParseException(lineNo, "Cant parse type");
    
    return outType;
}

static inline std::string stringFromCStringDelim(const char* c1,
                 size_t maxSize, char delim)
{
    size_t i = 0;
    for (i = 0; i < maxSize && c1[i] != delim; i++);
    return std::string(c1, i);
}

typedef std::map<std::string, InitKernelArgMapEntry> InitKernelArgMap;

static void parseAmdGpuKernelMetadata(const char* symName, size_t metadataSize,
          const char* kernelDesc, KernelInfo& kernelInfo)
{   /* parse kernel description */
    size_t lineNo = 1;
    size_t pos = 0;
    uint32_t argIndex = 0;
    
    InitKernelArgMap initKernelArgs;
    
    // first phase (value/pointer/image/sampler)
    while (pos < metadataSize)
    {
        if (kernelDesc[pos] != ';')
            throw ParseException(lineNo, "This is not KernelDesc line");
        pos++;
        if (pos >= metadataSize)
            throw ParseException(lineNo, "This is not KernelDesc line");
        
        size_t tokPos = pos;
        while (tokPos < metadataSize && kernelDesc[tokPos] != ':' &&
            kernelDesc[tokPos] != '\n') tokPos++;
        if (tokPos >= metadataSize)
            throw ParseException(lineNo, "Is not KernelDesc line");
        
        if (::strncmp(kernelDesc + pos, "value", tokPos-pos) == 0)
        { // value
            if (kernelDesc[tokPos] == '\n')
                throw ParseException(lineNo, "This is not value line");
            
            pos = ++tokPos;
            while (tokPos < metadataSize && kernelDesc[tokPos] != ':' &&
                kernelDesc[tokPos] != '\n') tokPos++;
            if (tokPos >= metadataSize || kernelDesc[tokPos] =='\n')
                throw ParseException(lineNo, "No separator after name");
            
            // extract arg name
            InitKernelArgMap::iterator argIt;
            {
                InitKernelArgMapEntry entry;
                entry.index = argIndex++;
                entry.namePos = pos;
                std::pair<InitKernelArgMap::iterator, bool> result = 
                    initKernelArgs.insert(std::make_pair(
                        std::string(kernelDesc+pos, tokPos-pos), entry));
                if (!result.second)
                    throw ParseException(lineNo, "Argument has been duplicated");
                argIt = result.first;
            }
            ///
            pos = ++tokPos;
            while (tokPos < metadataSize && kernelDesc[tokPos] != ':' &&
                kernelDesc[tokPos] != '\n') tokPos++;
            if (tokPos >= metadataSize || kernelDesc[tokPos] =='\n')
                throw ParseException(lineNo, "No separator after type");
            // get arg type
            const char* argType = kernelDesc + pos;
            ///
            if (tokPos - pos != 6 || ::strncmp(argType, "struct", 6)!=0)
            {   // regular type
                pos = ++tokPos;
                while (tokPos < metadataSize && kernelDesc[tokPos] != ':' &&
                    kernelDesc[tokPos] != '\n') tokPos++;
                if (tokPos >= metadataSize || kernelDesc[tokPos] =='\n')
                    throw ParseException(lineNo, "No separator after vector size");
                // get vector size
                {
                    const char* outEnd;
                    cxuint vectorSize = cstrtouiParse(kernelDesc + pos,
                          kernelDesc + tokPos, outEnd, ':', lineNo);
                    if (outEnd != kernelDesc + tokPos)
                        throw ParseException(lineNo, "Garbages after integer");
                    argIt->second.argType = determineKernelArgType(argType,
                           vectorSize, lineNo);
                }
                pos = tokPos;
            }
            else // if structure
                argIt->second.argType = KernelArgType::STRUCTURE;
        }
        else if (::strncmp(kernelDesc + pos, "pointer", tokPos-pos) == 0)
        { // pointer
            if (kernelDesc[tokPos] == '\n')
                throw ParseException(lineNo, "This is not pointer line");
            
            pos = ++tokPos;
            while (tokPos < metadataSize && kernelDesc[tokPos] != ':' &&
                kernelDesc[tokPos] != '\n') tokPos++;
            if (tokPos >= metadataSize || kernelDesc[tokPos] =='\n')
                throw ParseException(lineNo, "No separator after name");
            
            // extract arg name
            InitKernelArgMap::iterator argIt;
            {
                InitKernelArgMapEntry entry;
                entry.index = argIndex++;
                entry.namePos = pos;
                entry.argType = KernelArgType::POINTER;
                std::pair<InitKernelArgMap::iterator, bool> result = 
                    initKernelArgs.insert(std::make_pair(
                        std::string(kernelDesc+pos, tokPos-pos), entry));
                if (!result.second)
                    throw ParseException(lineNo, "Argument has been duplicated");
                argIt = result.first;
            }
            ///
            ++tokPos;
            for (cxuint k = 0; k < 4; k++) // // skip four fields
            {
                while (tokPos < metadataSize && kernelDesc[tokPos] != ':' &&
                    kernelDesc[tokPos] != '\n') tokPos++;
                if (tokPos >= metadataSize || kernelDesc[tokPos] =='\n')
                    throw ParseException(lineNo, "No separator after field");
                tokPos++;
            }
            pos = tokPos;
            // get pointer type (global/local/constant)
            if (pos+4 <= metadataSize && kernelDesc[pos] == 'u' && 
                kernelDesc[pos+1] == 'a' && kernelDesc[pos+2] == 'v' &&
                kernelDesc[pos+3] == ':')
            {
                argIt->second.ptrSpace = KernelPtrSpace::GLOBAL;
                pos += 4;
            }
            else if (pos+3 <= metadataSize && kernelDesc[pos] == 'h' &&
                kernelDesc[pos+1] == 'c' && kernelDesc[pos+2] == ':')
            {
                argIt->second.ptrSpace = KernelPtrSpace::CONSTANT;
                pos += 3;
            }
            else if (pos+3 <= metadataSize && kernelDesc[pos] == 'h' &&
                kernelDesc[pos+1] == 'l' && kernelDesc[pos+2] == ':')
            {
                argIt->second.ptrSpace = KernelPtrSpace::LOCAL;
                pos += 3;
            }
            else if (pos+2 <= metadataSize && kernelDesc[pos] == 'c' &&
                  kernelDesc[pos+1] == ':')
            {
                argIt->second.ptrSpace = KernelPtrSpace::CONSTANT;
                pos += 2;
            }
            else //if not match
                throw ParseException(lineNo, "Unknown pointer type");
        }
        else if (::strncmp(kernelDesc + pos, "image", tokPos-pos) == 0)
        { // image
            if (kernelDesc[tokPos] == '\n')
                throw ParseException(lineNo, "This is not image line");
            
            pos = ++tokPos;
            while (tokPos < metadataSize && kernelDesc[tokPos] != ':' &&
                kernelDesc[tokPos] != '\n') tokPos++;
            if (tokPos >= metadataSize || kernelDesc[tokPos] == '\n')
                throw ParseException(lineNo, "No separator after name");
            
            // extract arg name
            InitKernelArgMap::iterator argIt;
            {
                InitKernelArgMapEntry entry;
                entry.index = argIndex++;
                entry.namePos = pos;
                entry.ptrSpace = KernelPtrSpace::GLOBAL;
                entry.argType = KernelArgType::IMAGE; // set as image
                std::pair<InitKernelArgMap::iterator, bool> result = 
                    initKernelArgs.insert(std::make_pair(
                        std::string(kernelDesc+pos, tokPos-pos), entry));
                if (!result.second)
                    throw ParseException(lineNo, "Argument has been duplicated");
                argIt = result.first;
            }
            
            pos = ++tokPos; // skip next field
            while (tokPos < metadataSize && kernelDesc[tokPos] != ':' &&
                kernelDesc[tokPos] != '\n') tokPos++;
            if (tokPos >= metadataSize || kernelDesc[tokPos] == '\n')
                throw ParseException(lineNo, "No separator after field");
            
            pos = ++tokPos;
            if (pos+3 > metadataSize || kernelDesc[pos+2] != ':')
                throw ParseException(lineNo, "Cant parse image access qualifier");
            
            if (kernelDesc[pos] == 'R' && kernelDesc[pos+1] == 'O')
                argIt->second.ptrAccess |= KARG_PTR_READ_ONLY;
            else if (kernelDesc[pos] == 'W' && kernelDesc[pos+1] == 'O')
                argIt->second.ptrAccess |= KARG_PTR_WRITE_ONLY;
            else
                throw ParseException(lineNo, "Cant parse image access qualifier");
            pos += 3;
        }
        else if (::strncmp(kernelDesc + pos, "sampler", tokPos-pos) == 0)
        { // sampler (set up some argument as sampler
            if (kernelDesc[tokPos] == '\n')
                throw ParseException(lineNo, "This is not sampler line");
            
            pos = ++tokPos;
            while (tokPos < metadataSize && kernelDesc[tokPos] != ':' &&
                kernelDesc[tokPos] != '\n') tokPos++;
            if (tokPos >= metadataSize)
                throw ParseException(lineNo, "No separator after name");
            
            std::string argName(kernelDesc+pos, tokPos-pos);
            InitKernelArgMap::iterator argIt = initKernelArgs.find(argName);
            if (argIt != initKernelArgs.end())
            {
                argIt->second.origArgType = argIt->second.argType;
                argIt->second.argType = KernelArgType::SAMPLER;
            }
            
            pos = tokPos;
        }
        else if (::strncmp(kernelDesc + pos, "constarg", tokPos-pos) == 0)
        {
            if (kernelDesc[tokPos] == '\n')
                throw ParseException(lineNo, "This is not constarg line");
            
            pos = ++tokPos;
            // skip number
            while (tokPos < metadataSize && kernelDesc[tokPos] != ':' &&
                kernelDesc[tokPos] != '\n') tokPos++;
            if (tokPos >= metadataSize || kernelDesc[tokPos] == '\n')
                throw ParseException(lineNo, "No separator after field");
            
            pos = ++tokPos;
            while (tokPos < metadataSize && kernelDesc[tokPos] != '\n') tokPos++;
            if (tokPos >= metadataSize)
                throw ParseException(lineNo, "End of data");
            
            /// put constant
            const std::string thisName(kernelDesc+pos, tokPos-pos);
            InitKernelArgMap::iterator argIt = initKernelArgs.find(thisName);
            if (argIt == initKernelArgs.end())
                throw ParseException(lineNo, "Cant find constant argument");
            // set up const access type
            argIt->second.ptrAccess |= KARG_PTR_CONST;
            pos = tokPos;
        }
        else if (::strncmp(kernelDesc + pos, "reflection", tokPos-pos) == 0)
        {
            if (kernelDesc[tokPos] == '\n')
                throw ParseException(lineNo, "This is not reflection line");
            break;
        }
        
        while (pos < metadataSize && kernelDesc[pos] != '\n') pos++;
        lineNo++;
        pos++; // skip newline
    }
    
    kernelInfo.kernelName.assign(symName+9, ::strlen(symName)-18);
    kernelInfo.allocateArgs(argIndex);
    
    for (const auto& e: initKernelArgs)
    {   /* initialize kernel arguments before set argument type from reflections */
        KernelArg& karg = kernelInfo.argInfos[e.second.index];
        karg.argType = e.second.argType;
        karg.ptrSpace = e.second.ptrSpace;
        karg.ptrAccess = e.second.ptrAccess;
        karg.argName = stringFromCStringDelim(kernelDesc + e.second.namePos,
                  metadataSize-e.second.namePos, ':');
    }
    
    if (argIndex != 0)
    {   /* check whether not end */
        if (pos >= metadataSize)
            throw ParseException(lineNo, "Unexpected end of data");
        
        // reflections
        pos--; // skip ';'
        while (pos < metadataSize)
        {
            if (kernelDesc[pos] != ';')
                throw ParseException(lineNo, "Is not KernelDesc line");
            pos++;
            if (pos >= metadataSize)
                throw ParseException(lineNo, "Is not KernelDesc line");
            
            if (pos+11 > metadataSize ||
                ::strncmp(kernelDesc+pos, "reflection:", 11) != 0)
                break; // end!!!
            pos += 11;
            
            size_t tokPos = pos;
            while (tokPos < metadataSize && kernelDesc[tokPos] != ':' &&
                kernelDesc[tokPos] != '\n') tokPos++;
            if (tokPos >= metadataSize || kernelDesc[tokPos] == '\n')
                throw ParseException(lineNo, "Is not KernelDesc line");
            
            const char* outEnd;
            cxuint argIndex = cstrtouiParse(kernelDesc+pos, kernelDesc+tokPos,
                            outEnd, ':', lineNo);
            
            if (outEnd != kernelDesc + tokPos)
                throw ParseException(lineNo, "Garbages after integer");
            if (argIndex >= kernelInfo.argsNum)
                throw ParseException(lineNo, "Argument index out of range");
            pos = tokPos+1;
            
            KernelArg& argInfo = kernelInfo.argInfos[argIndex];
            argInfo.typeName = stringFromCStringDelim(
                kernelDesc+pos, metadataSize-pos, '\n');
            
            if (argInfo.argName.compare(0, 8, "unknown_") == 0 &&
                argInfo.typeName != "sampler_t" &&
                argInfo.argType == KernelArgType::SAMPLER)
            {   InitKernelArgMap::const_iterator argIt =
                            initKernelArgs.find(argInfo.argName);
                if (argIt != initKernelArgs.end() &&
                    argIt->second.origArgType != KernelArgType::VOID)
                {   /* revert sampler type and restore original arg type */
                    argInfo.argType = argIt->second.origArgType;
                }
            }    
            
            while (pos < metadataSize && kernelDesc[pos] != '\n') pos++;
            lineNo++;
            pos++;
        }
    }
}

struct AmdGPU32Types : Elf32Types
{
    typedef ElfBinary32 ElfBinary;
};

struct AmdGPU64Types: Elf64Types
{
    typedef ElfBinary64 ElfBinary;
};

template<typename Types>
static size_t initKernelInfos(const typename Types::ElfBinary& elf,
         cxuint creationFlags, KernelInfo*& kernelInfos,
         const std::vector<size_t>& metadataSyms,
         AmdMainBinaryBase::KernelInfoMap& kernelInfosMap)
{
    size_t kernelInfosNum = 0;
    
    try
    {
        kernelInfos = new KernelInfo[metadataSyms.size()];
        
        typename Types::Size ki = 0;
        for (typename Types::Size it: metadataSyms)
        {   // read symbol _OpenCL..._metadata
            const typename Types::Sym& sym = elf.getSymbol(it);
            const char* symName = elf.getSymbolName(it);
            if (sym.st_shndx >= elf.getSectionHeadersNum())
                throw Exception("Metadata section index out of range");
            
            const typename Types::Shdr& rodataHdr = elf.getSectionHeader(sym.st_shndx);
            const char* rodataContent = elf.getBinaryCode() + rodataHdr.sh_offset;
            
            if (ULEV(sym.st_value) > ULEV(rodataHdr.sh_size))
                throw Exception("Metadata offset out of range");
            if (ULEV(sym.st_value)+ULEV(sym.st_size) > ULEV(rodataHdr.sh_size))
                throw Exception("Metadata offset+size out of range");
            
            parseAmdGpuKernelMetadata(symName, ULEV(sym.st_size),
                      rodataContent + ULEV(sym.st_value), kernelInfos[ki]);
            ki++;
        }
        kernelInfosNum = metadataSyms.size();
        /* maps kernel info */
        if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
            for (size_t i = 0; i < kernelInfosNum; i++)
                kernelInfosMap.insert(
                    std::make_pair(kernelInfos[i].kernelName, i));
    }
    catch(...)
    {
        delete[] kernelInfos;
        kernelInfos = nullptr;
        throw;
    }
    return kernelInfosNum;
}

template<typename Types>
static void initInnerBinaries(typename Types::ElfBinary& elf,
         cxuint creationFlags, AmdInnerGPUBinary32*& innerBinaries,
         size_t innerBinariesNum, const std::vector<size_t>& choosenSyms,
         uint16_t textIndex, AmdMainGPUBinary32::InnerBinaryMap& innerBinaryMap)
{
    const typename Types::Shdr& textHdr = elf.getSectionHeader(textIndex);
    char* textContent = elf.getBinaryCode() + textHdr.sh_offset;
    
    /* create table of innerBinaries */
    innerBinaries = new AmdInnerGPUBinary32[innerBinariesNum];
    size_t ki = 0;
    for (auto it: choosenSyms)
    {
        const char* symName = elf.getSymbolName(it);
        size_t len = ::strlen(symName);
        const typename Types::Sym& sym = elf.getSymbol(it);
        
        if (ULEV(sym.st_value) > ULEV(textHdr.sh_size))
            throw Exception("Inner binary offset out of range!");
        if (ULEV(sym.st_value) + ULEV(sym.st_size) > ULEV(textHdr.sh_size))
            throw Exception("Inner binary offset+size out of range!");
        
        innerBinaries[ki++] = AmdInnerGPUBinary32(std::string(symName+9, len-16),
            ULEV(sym.st_size), textContent+ULEV(sym.st_value),
            (creationFlags >> AMDBIN_INNER_SHIFT) & ELF_CREATE_ALL);
    }
    if ((creationFlags & AMDBIN_CREATE_INNERBINMAP) != 0)
        for (size_t i = 0; i < innerBinariesNum; i++)
            innerBinaryMap.insert(std::make_pair(innerBinaries[i].getKernelName(), i));
}

/* AmdMainGPUBinary32 */

AmdMainGPUBinary32::AmdMainGPUBinary32(size_t binaryCodeSize, char* binaryCode,
       cxuint creationFlags) : AmdMainBinaryBase(AmdMainType::GPU_BINARY),
          ElfBinary32(binaryCodeSize, binaryCode, creationFlags),
          innerBinariesNum(0), innerBinaries(nullptr)
{
    cxuint textIndex = SHN_UNDEF;
    try
    { textIndex = getSectionIndex(".text"); }
    catch(const Exception& ex)
    { } // ignore failed
    
    std::vector<size_t> choosenSyms;
    std::vector<size_t> choosenSymsMetadata;
    
    const bool doKernelInfo = (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0;
    
    for (uint32_t i = 0; i < symbolsNum; i++)
    {
        const char* symName = getSymbolName(i);
        size_t len = ::strlen(symName);
        if (len < 16 || ::strncmp(symName, "__OpenCL_", 9) != 0)
            continue;
        
        if (::strcmp(symName+len-7, "_kernel") == 0) // if kernel
            choosenSyms.push_back(i);
        if (doKernelInfo && len >= 18 &&
            ::strcmp(symName+len-9, "_metadata") == 0) // if metadata
            choosenSymsMetadata.push_back(i);
    }
    innerBinariesNum = choosenSyms.size();
    
    try
    {
    if (textIndex != SHN_UNDEF) /* if have ".text" */
        initInnerBinaries<AmdGPU32Types>(*this, creationFlags, innerBinaries,
                         innerBinariesNum, choosenSyms, textIndex, innerBinaryMap);
    
    if ((creationFlags & AMDBIN_CREATE_KERNELINFO) != 0)
        kernelInfosNum = initKernelInfos<AmdGPU32Types>(*this, creationFlags,
                        kernelInfos, choosenSymsMetadata, kernelInfosMap);
    }
    catch(...)
    {   /* free arrays */
        delete[] innerBinaries;
        delete[] kernelInfos;
        throw;
    }
}

AmdMainGPUBinary32::~AmdMainGPUBinary32()
{
    delete[] innerBinaries;
}

const AmdInnerGPUBinary32& AmdMainGPUBinary32::getInnerBinary(const char* name) const
{
    InnerBinaryMap::const_iterator it = innerBinaryMap.find(name);
    if (it == innerBinaryMap.end())
        throw Exception("Cant find inner binary");
    return innerBinaries[it->second];
}

/* AmdMainGPUBinary64 */

AmdMainGPUBinary64::AmdMainGPUBinary64(size_t binaryCodeSize, char* binaryCode,
       cxuint creationFlags) : AmdMainBinaryBase(AmdMainType::GPU_64_BINARY),
          ElfBinary64(binaryCodeSize, binaryCode, creationFlags),
          innerBinariesNum(0), innerBinaries(nullptr)
{
    cxuint textIndex = SHN_UNDEF;
    try
    { textIndex = getSectionIndex(".text"); }
    catch(const Exception& ex)
    { } // ignore failed
    
    std::vector<size_t> choosenSyms;
    std::vector<size_t> choosenSymsMetadata;
    
    const bool doKernelInfo = (creationFlags & AMDBIN_CREATE_KERNELINFO) != 0;
    
    for (size_t i = 0; i < symbolsNum; i++)
    {
        const char* symName = getSymbolName(i);
        size_t len = ::strlen(symName);
        if (len < 16 || ::strncmp(symName, "__OpenCL_", 9) != 0)
            continue;
        
        if (::strcmp(symName+len-7, "_kernel") == 0) // if kernel
            choosenSyms.push_back(i);
        if (doKernelInfo && len >= 18 &&
            ::strcmp(symName+len-9, "_metadata") == 0) // if metadata
            choosenSymsMetadata.push_back(i);
    }
    innerBinariesNum = choosenSyms.size();
    
    try
    {
    if (textIndex != SHN_UNDEF) /* if have ".text" */
        initInnerBinaries<AmdGPU64Types>(*this, creationFlags, innerBinaries,
                         innerBinariesNum, choosenSyms, textIndex, innerBinaryMap);
    
    if ((creationFlags & AMDBIN_CREATE_KERNELINFO) != 0)
        kernelInfosNum = initKernelInfos<AmdGPU64Types>(*this, creationFlags,
                        kernelInfos, choosenSymsMetadata, kernelInfosMap);;
    }
    catch(...)
    {   /* free arrays */
        delete[] innerBinaries;
        delete[] kernelInfos;
        throw;
    }
}

AmdMainGPUBinary64::~AmdMainGPUBinary64()
{
    delete[] innerBinaries;
}

const AmdInnerGPUBinary32& AmdMainGPUBinary64::getInnerBinary(const char* name) const
{
    InnerBinaryMap::const_iterator it = innerBinaryMap.find(name);
    if (it == innerBinaryMap.end())
        throw Exception("Cant find inner binary");
    return innerBinaries[it->second];
}

/* AmdMainX86Binary32 */

void AmdMainX86Binary32::initKernelInfos(cxuint creationFlags)
{
    kernelInfosNum = innerBinary.getKernelInfos(kernelInfos);
    if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
        for (size_t i = 0; i < kernelInfosNum; i++)
            kernelInfosMap.insert(
                std::make_pair(kernelInfos[i].kernelName, i));
}

AmdMainX86Binary32::AmdMainX86Binary32(size_t binaryCodeSize, char* binaryCode,
       cxuint creationFlags) : AmdMainBinaryBase(AmdMainType::X86_BINARY),
       ElfBinary32(binaryCodeSize, binaryCode, creationFlags)
{
    cxuint textIndex = SHN_UNDEF;
    try
    { textIndex = getSectionIndex(".text"); }
    catch(const Exception& ex)
    { } // ignore failed
    
    if (textIndex != SHN_UNDEF)
    {
        const Elf32_Shdr& textHdr = getSectionHeader(textIndex);
        char* textContent = binaryCode + ULEV(textHdr.sh_offset);
        
        innerBinary = AmdInnerX86Binary32(ULEV(textHdr.sh_size), textContent,
                (creationFlags >> AMDBIN_INNER_SHIFT) & ELF_CREATE_ALL);
    }
    if ((creationFlags & AMDBIN_CREATE_KERNELINFO) != 0)
        initKernelInfos(creationFlags);
}

/* AmdMainX86Binary64 */

void AmdMainX86Binary64::initKernelInfos(cxuint creationFlags)
{
    kernelInfosNum = innerBinary.getKernelInfos(kernelInfos);
    if ((creationFlags & AMDBIN_CREATE_KERNELINFOMAP) != 0)
        for (size_t i = 0; i < kernelInfosNum; i++)
            kernelInfosMap.insert(
                std::make_pair(kernelInfos[i].kernelName, i));
}

AmdMainX86Binary64::AmdMainX86Binary64(size_t binaryCodeSize, char* binaryCode,
       cxuint creationFlags) : AmdMainBinaryBase(AmdMainType::X86_64_BINARY),
       ElfBinary64(binaryCodeSize, binaryCode, creationFlags)
{
    cxuint textIndex = SHN_UNDEF;
    try
    { textIndex = getSectionIndex(".text"); }
    catch(const Exception& ex)
    { } // ignore failed
    
    if (textIndex != SHN_UNDEF)
    {
        const Elf64_Shdr& textHdr = getSectionHeader(".text");
        char* textContent = binaryCode + ULEV(textHdr.sh_offset);
        
        innerBinary = AmdInnerX86Binary64(ULEV(textHdr.sh_size), textContent,
                (creationFlags >> AMDBIN_INNER_SHIFT) & ELF_CREATE_ALL);
    }
    if ((creationFlags & AMDBIN_CREATE_KERNELINFO) != 0)
        initKernelInfos(creationFlags);
}

/* create amd binary */

AmdMainBinaryBase* CLRX::createAmdBinaryFromCode(size_t binaryCodeSize, char* binaryCode,
        cxuint creationFlags)
{
    if (ULEV(*reinterpret_cast<const uint32_t*>(binaryCode)) != elfMagicValue)
        throw Exception("This is not ELF binary");
    if (binaryCode[EI_DATA] != ELFDATA2LSB)
        throw Exception("Other than little-endian binaries are not supported!");
    
    if (binaryCode[EI_CLASS] == ELFCLASS32)
    {
        const Elf32_Ehdr* ehdr = reinterpret_cast<const Elf32_Ehdr*>(binaryCode);
        if (ULEV(ehdr->e_machine) != ELF_M_X86) //if gpu
            return new AmdMainGPUBinary32(binaryCodeSize, binaryCode, creationFlags);
        return new AmdMainX86Binary32(binaryCodeSize, binaryCode, creationFlags);
    }
    else if (binaryCode[EI_CLASS] == ELFCLASS64)
    {
        const Elf64_Ehdr* ehdr = reinterpret_cast<const Elf64_Ehdr*>(binaryCode);
        if (ULEV(ehdr->e_machine) != ELF_M_X86)
            return new AmdMainGPUBinary64(binaryCodeSize, binaryCode, creationFlags);
        return new AmdMainX86Binary64(binaryCodeSize, binaryCode, creationFlags);
    }
    else // fatal error
        throw Exception("Unsupported ELF class");
}
