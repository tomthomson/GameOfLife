#ifndef KERNELFILE_HPP_
#define KERNELFILE_HPP_

#include <vector>
#include <string>
#include <fstream>

//! Get current directory
extern std::string getCurrentDir();

class KernelFile {
private:
	// source code of the CL program
	std::string source_;

public:
    // Default constructor
    KernelFile(): source_("") {}

    // Destructor
    ~KernelFile() {};

    // Opens the CL program file
    bool open(const char* fileName);

    // Returns a pointer to the string object with the source code
    const std::string& source() const { return source_; }

private:
    // Disable copy constructor
    KernelFile(const KernelFile&);

    // Disable operator=
    KernelFile& operator=(const KernelFile&);
};

#endif
