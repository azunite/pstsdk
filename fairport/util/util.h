//! \file
//! \brief General utility functions and classes
//! \author Terry Mahaffey
//!
//! This is where any generalized utility classes and functions go which
//! are not directly related to MS-PST in some fashion. It is my hope that
//! this file stay as small as possible.
//! \ingroup util

#ifndef FAIRPORT_UTIL_UTIL_H
#define FAIRPORT_UTIL_UTIL_H

#include <cstdio>
#include <time.h>
#include <memory>
#include <vector>
#include <boost/utility.hpp>

#include "fairport/util/errors.h"
#include "fairport/util/primatives.h"

namespace fairport
{

//! \brief A generic class to read and write to a file
//!
//! This was necessary to get around the 32 bit limit (4GB) file size
//! limitation in ANSI C++. I needed to use compiler specific work arounds,
//! and that logic is centralized here.
//! \ingroup util
class file : private boost::noncopyable
{
public:
    //! \brief Construct a file object from the given filename
    //! \throw runtime_error if an error occurs opening the file
    //! \param[in] filename The file to open
    file(const std::wstring& filename);
    
    //! \brief Close the file
    ~file();
    
    //! \brief Move constructor
    //! \param[in] other file object to move from
    file(file&& other);

    //! \brief Move assignment operator
    //! \param[in] other file object to move from
    file& operator=(file&& other);

    //! \brief Read from the file
    //! \throw out_of_range if the requested location or location+size is past EOF
    //! \param[in,out] buffer The buffer to store the data in. The size of this vector is the amount of data to read.
    //! \param[in] offset The location on disk to read the data from.
    //! \returns The amount of data read
    size_t read(std::vector<byte>& buffer, ulonglong offset) const;

    //! \brief Write to the file
    //! \throw out_of_range if the requested location or location+size is past EOF
    //! \param[in] buffer The data to write. The size of this vector is the amount of data to write.
    //! \param[in] offset The location on disk to read the data from.
    //! \returns The amount of data written
    size_t write(const std::vector<byte>& buffer, ulonglong offset);

private:
    std::wstring m_filename;    //!< The filename used to open this file
    FILE * m_pfile;             //!< The file pointer
};


//! \brief Convert from a filetime to time_t
//!
//! FILETIME is a Win32 date/time type representing the number of 100 ns
//! intervals since Jan 1st, 1601. This function converts it to a standard
//! time_t.
//! \param[in] filetime The time to convert from
//! \returns filetime, as a time_t.
//! \ingroup util
time_t filetime_to_time_t(ulonglong filetime);

//! \brief Convert from a time_t to filetime
//!
//! FILETIME is a Win32 date/time type representing the number of 100 ns
//! intervals since Jan 1st, 1601. This function converts to it from a 
//! standard time_t.
//! \param[in] time The time to convert from
//! \returns time, as a filetime.
//! \ingroup util
ulonglong time_t_to_filetime(time_t time);

//! \brief Convert from a VT_DATE to a time_t
//!
//! VT_DATE is an OLE date/time type where the integer part is the date and
//! the fraction part is the time. This function converts it to a standard
//! time_t
//! \param[in] vt_time The time to convert from
//! \returns vt_time, as a time_t
//! \ingroup util
time_t vt_date_to_time_t(double vt_time);

//! \brief Convert from a time_t to a VT_DATE
//!
//! VT_DATE is an OLE date/time type where the integer part is the date and
//! the fraction part is the time. This function converts to it from a 
//! standard time_t.
//! \param[in] time The time to convert from
//! \returns time, as a VT_DATE
//! \ingroup util
double time_t_to_vt_date(time_t time);

//! \brief Test to see if the specified bit in the buffer is set
//! \param[in] pbytes The buffer to check
//! \param[in] bit Which bit to test
//! \returns true if the specified bit is set, false if not
//! \ingroup util
bool test_bit(const byte* pbytes, ulong bit);

} // end fairport namespace

inline fairport::file::file(const std::wstring& filename)
: m_filename(filename)
{
    const char* mode = "rb";

#ifdef _MSC_VER 
    errno_t err = fopen_s(&m_pfile, std::string(filename.begin(), filename.end()).c_str(), mode);
    if(err != 0)
        m_pfile = NULL;
#else
    m_pfile = fopen(std::string(filename.begin(), filename.end()).c_str(), mode);
#endif
    if(m_pfile == NULL)
        throw std::runtime_error("fopen failed");
}

inline fairport::file::file(file&& other)
: m_filename(std::move(other.m_filename)), m_pfile(other.m_pfile)
{
    other.m_pfile = NULL;
}

inline fairport::file& fairport::file::operator=(file&& other)
{
    std::swap(m_pfile, other.m_pfile);
    std::swap(m_filename, other.m_filename);

    return *this;
}

inline fairport::file::~file()
{
    fflush(m_pfile);
    fclose(m_pfile);
}

inline size_t fairport::file::read(std::vector<byte>& buffer, ulonglong offset) const
{
#ifdef _MSC_VER
    if(_fseeki64(m_pfile, offset, SEEK_SET) != 0)
#else
    if(fseek(m_pfile, offset, SEEK_SET) != 0)
#endif
    {
        throw std::out_of_range("fseek failed");
    }

    size_t read = fread(&buffer[0], 1, buffer.size(), m_pfile);

    if(read != buffer.size())
        throw std::out_of_range("fread failed");

    return read;
}

inline size_t fairport::file::write(const std::vector<byte>& buffer, ulonglong offset)
{
#ifdef _MSC_VER
    if(_fseeki64(m_pfile, offset, SEEK_SET) != 0)
#else
    if(fseek(m_pfile, offset, SEEK_SET) != 0)
#endif
    {
        throw std::out_of_range("fseek failed");
    }

    size_t write = fwrite(&buffer[0], 1, buffer.size(), m_pfile);

    if(write != buffer.size())
        throw std::out_of_range("fwrite failed");

    return write;
}

inline time_t fairport::filetime_to_time_t(ulonglong filetime)
{
    ulonglong jan1970 = 116444736000000000ULL;

    return (filetime - jan1970) / 10000000;
}

inline fairport::ulonglong fairport::time_t_to_filetime(time_t time)
{
    ulonglong jan1970 = 116444736000000000ULL;

    return (time * 10000000) + jan1970;
}

inline time_t fairport::vt_date_to_time_t(double)
{
    throw not_implemented("vt_date_to_time_t");
}

inline double fairport::time_t_to_vt_date(time_t)
{
    throw not_implemented("vt_date_to_time_t");
}

inline bool fairport::test_bit(const byte* pbytes, ulong bit)
{
    return (*(pbytes + (bit >> 3)) & (0x80 >> (bit & 7))) != 0;
}

#endif
