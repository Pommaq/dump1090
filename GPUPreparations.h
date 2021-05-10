//
// Created by Timmy on 4/15/2021.
//

#ifndef DUMP1090_GPUPREPARATIONS_H
#define DUMP1090_GPUPREPARATIONS_H

#include <iostream>
#include <fstream>
#include <string>
#include <OpenCL/OpenCL.h> // TODO: Fix this import.
#include <CL/cl.h>

std::string readcode(std::string filename){
    // TODO: Avoid unnecessary copies with std::move
    std::string buffer;
    std::ifstream code(filename, 'r');
    if (code){
        while (code >> buffer);
    }
    else{
        std::cout << "file " << filename << " Not found!" << std::endl;
    }
    return buffer;
}

cl_int Populate_Devices(){
    /* Created with help from http://developer.amd.com/wordpress/media/2013/01/Introduction_to_OpenCL_Programming-Training_Guide-201005.pdf
    * */
    /* Used to populate the global struct Devices with required data*/
    g_OpenCL_Dat.bitf_output_array = malloc(sizeof(cl_int) * MODES_LONG_MSG_BITS);
    cl_platform_id platform;
    cl_uint num_platforms;
    cl_int err[5];
    err[0] = clGetPlatformIDs(1, &platform, &num_platforms);



    if (err[0] != CL_SUCCESS){
        fprintf(stderr, "No OpenCL capable platform found\n");
        free_resources();
        return err[0];
    }

    /* Find the ID of a OpenCL capable GPU*/
    cl_uint num_devices;
    err[0] = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &(g_OpenCL_Dat.device), &num_devices);
    if (err[0] != CL_SUCCESS){
        switch (err[0]){
            case CL_INVALID_PLATFORM:
                fprintf(stderr, "Invalid platform\n");
                break;
            case CL_INVALID_DEVICE_TYPE:
                fprintf(stderr, "Invalid device type\n");
                break;
            case CL_INVALID_VALUE:
                fprintf(stderr, "device_id and num_devices have an invalid value\n");
                break;
            case CL_DEVICE_NOT_FOUND:
                fprintf(stderr, "No OpenCL capable GPUs found\n");
                break;
            default:
                fprintf(stderr, "Something went wrong when attempting to find a GPU\n");
                break;
        }
        free_resources();
        return err[0];
    }

    cl_context_properties properties[] = {CL_CONTEXT_PLATFORM, (cl_context_properties) platform, 0};
    g_OpenCL_Dat.context = clCreateContext(properties,1, &(g_OpenCL_Dat.device), NULL, NULL, err);
    if (err[0] != CL_SUCCESS){
        switch (err[0]){
            case CL_DEVICE_NOT_AVAILABLE:
                fprintf(stderr, "GPU is currently unavailable\n");
                break;
            case CL_OUT_OF_HOST_MEMORY: // Host refers to the pc running this program. not the GPU
                fprintf(stderr, "Host is unable to allocate OpenCL resources\n");
                break;
            default:
                fprintf(stderr, "Something went wrong when creating a device context\n");
                break;
        }
        free_resources();
        return err[0];
    }

    // consider setting CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE
    // Using this version will limit us to devices supporting OpenCl 2.0 and newer, use deprecated clCreateCommandQueue() to support older devices.
    g_OpenCL_Dat.queue = clCreateCommandQueueWithProperties(g_OpenCL_Dat.context, g_OpenCL_Dat.device, 0, err);
    if (err[0] != CL_SUCCESS){
        switch (err[0]){
            case CL_INVALID_QUEUE_PROPERTIES:
                fprintf(stderr, "Device does not support set properties\n");
                break;
            case CL_OUT_OF_HOST_MEMORY:
                fprintf(stderr, "Host unable to allocate OpenCL resources\n");
                break;
            default:
                fprintf(stderr, "Something went wrong when creating the commandqueue\n");
                break;
        }
        free_resources();
        return err[0];
    }

    // Success! We have a valid context and a commandqueue to feed it instructions! Creating and compiling the programs...
    const char *SingleBitErrorCode = "//\n"
                                     "// Created by timmy on 2021-02-23.\n"
                                     "//\n"
                                     "\n"
                                     "\n"
                                     "\n"
                                     "__constant int modes_checksum_table[112] = {\n"
                                     "        0x3935ea, 0x1c9af5, 0xf1b77e, 0x78dbbf, 0xc397db, 0x9e31e9, 0xb0e2f0, 0x587178,\n"
                                     "        0x2c38bc, 0x161c5e, 0x0b0e2f, 0xfa7d13, 0x82c48d, 0xbe9842, 0x5f4c21, 0xd05c14,\n"
                                     "        0x682e0a, 0x341705, 0xe5f186, 0x72f8c3, 0xc68665, 0x9cb936, 0x4e5c9b, 0xd8d449,\n"
                                     "        0x939020, 0x49c810, 0x24e408, 0x127204, 0x093902, 0x049c81, 0xfdb444, 0x7eda22,\n"
                                     "        0x3f6d11, 0xe04c8c, 0x702646, 0x381323, 0xe3f395, 0x8e03ce, 0x4701e7, 0xdc7af7,\n"
                                     "        0x91c77f, 0xb719bb, 0xa476d9, 0xadc168, 0x56e0b4, 0x2b705a, 0x15b82d, 0xf52612,\n"
                                     "        0x7a9309, 0xc2b380, 0x6159c0, 0x30ace0, 0x185670, 0x0c2b38, 0x06159c, 0x030ace,\n"
                                     "        0x018567, 0xff38b7, 0x80665f, 0xbfc92b, 0xa01e91, 0xaff54c, 0x57faa6, 0x2bfd53,\n"
                                     "        0xea04ad, 0x8af852, 0x457c29, 0xdd4410, 0x6ea208, 0x375104, 0x1ba882, 0x0dd441,\n"
                                     "        0xf91024, 0x7c8812, 0x3e4409, 0xe0d800, 0x706c00, 0x383600, 0x1c1b00, 0x0e0d80,\n"
                                     "        0x0706c0, 0x038360, 0x01c1b0, 0x00e0d8, 0x00706c, 0x003836, 0x001c1b, 0xfff409,\n"
                                     "        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,\n"
                                     "        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,\n"
                                     "        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000\n"
                                     "};\n"
                                     "\n"
                                     "__constant bitmasks[] = {\n"
                                     "        0b10000000, 0b1000000, 0b100000, 0b10000, 0b1000, 0b100, 0b10, 0b1\n"
                                     "};\n"
                                     "int modesChecksum(unsigned char *msg, int bits) {\n"
                                     "    int crc = 0;\n"
                                     "    int offset = (bits == 112) ? 0 : (112-56);\n"
                                     "    int j;\n"
                                     "    // TODO: Consider running this in separate workgroups aswell?\n"
                                     "    for(j = 0; j < bits; j++) {\n"
                                     "        int byte = j/8;\n"
                                     "//        int bit = j%8;\n"
                                     "        int bitmask = bitmasks[j%8];\n"
                                     "\n"
                                     "        /* If bit is set, xor with corresponding table entry. */\n"
                                     "        if (msg[byte] & bitmask)\n"
                                     "            crc ^= modes_checksum_table[j+offset];\n"
                                     "    }\n"
                                     "    return crc; /* 24 bit checksum. */\n"
                                     "}\n"
                                     "\n"
                                     "__constant int MODES_LONG_MSG_BITS = 112;\n"
                                     "\n"
                                     "/* Try to fix single bit errors using the checksum. On success modifies\n"
                                     " * the original buffer with the fixed version, and saves the position\n"
                                     " * of the error bit. Otherwise if fixing failed nothing is saved leaving returnval as-is. */\n"
                                     "__kernel void fixSingleBitErrors(__global unsigned char* msg, __global int* bits,  __global  int* returnval){\n"
                                     "    size_t j = get_global_id(0);\n"
                                     "    unsigned char aux[MODES_LONG_MSG_BITS/8]; // NOTE: Might have to allocate in main memory and give to pointer.\n"
                                     "    size_t byte = j/8;\n"
                                     "    int bitmask = 1 << (7 - (j % 8));\n"
                                     "    int crc1, crc2;\n"
                                     "\n"
                                     "    for (int i = 0; i < (*bits)/8; i+=2){\n"
                                     "        aux[i] = msg[i];\n"
                                     "        aux[i+1] = msg[i+1];\n"
                                     "    }\n"
                                     "\n"
                                     "    aux[byte] ^= bitmask; /* Flip the J-th bit*/\n"
                                     "\n"
                                     "    crc1 = ((uint)aux[(*bits/8)-3] << 16) |\n"
                                     "           ((uint)aux[(*bits/8)-2] << 8) |\n"
                                     "           (uint)aux[(*bits/8)-1];\n"
                                     "    crc2 = modesChecksum(aux,*bits);\n"
                                     "    //barrier(CLK_LOCAL_MEM_FENCE);\n"
                                     "    if (crc1 == crc2){\n"
                                     "        /* The error is fixed. Overwrite the original buffer with\n"
                                     "             * the corrected sequence, and returns the error bit\n"
                                     "             * position.\n"
                                     "             * This should only happen in one work-unit if the number of errors in the bitf_message was few enough. I see no problem if multiple units finds a solution and overwrites each other however.\n"
                                     "             * */\n"
                                     "        for (int i = 0; i < (*bits)/8; i+=2){\n"
                                     "            msg[i] = aux[i];\n"
                                     "            msg[i+1] = aux[i+1];\n"
                                     "        }\n"
                                     "        returnval[j] = j;\n"
                                     "    }\n"
                                     "    // if we couldn't fix it, dont touch returnval.\n"
                                     "}\n"
                                     "";

    const char *TwoBitErrorCode ="__constant uint modes_checksum_table[112] = {\n"
                                 "        0x3935ea, 0x1c9af5, 0xf1b77e, 0x78dbbf, 0xc397db, 0x9e31e9, 0xb0e2f0, 0x587178,\n"
                                 "        0x2c38bc, 0x161c5e, 0x0b0e2f, 0xfa7d13, 0x82c48d, 0xbe9842, 0x5f4c21, 0xd05c14,\n"
                                 "        0x682e0a, 0x341705, 0xe5f186, 0x72f8c3, 0xc68665, 0x9cb936, 0x4e5c9b, 0xd8d449,\n"
                                 "        0x939020, 0x49c810, 0x24e408, 0x127204, 0x093902, 0x049c81, 0xfdb444, 0x7eda22,\n"
                                 "        0x3f6d11, 0xe04c8c, 0x702646, 0x381323, 0xe3f395, 0x8e03ce, 0x4701e7, 0xdc7af7,\n"
                                 "        0x91c77f, 0xb719bb, 0xa476d9, 0xadc168, 0x56e0b4, 0x2b705a, 0x15b82d, 0xf52612,\n"
                                 "        0x7a9309, 0xc2b380, 0x6159c0, 0x30ace0, 0x185670, 0x0c2b38, 0x06159c, 0x030ace,\n"
                                 "        0x018567, 0xff38b7, 0x80665f, 0xbfc92b, 0xa01e91, 0xaff54c, 0x57faa6, 0x2bfd53,\n"
                                 "        0xea04ad, 0x8af852, 0x457c29, 0xdd4410, 0x6ea208, 0x375104, 0x1ba882, 0x0dd441,\n"
                                 "        0xf91024, 0x7c8812, 0x3e4409, 0xe0d800, 0x706c00, 0x383600, 0x1c1b00, 0x0e0d80,\n"
                                 "        0x0706c0, 0x038360, 0x01c1b0, 0x00e0d8, 0x00706c, 0x003836, 0x001c1b, 0xfff409,\n"
                                 "        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,\n"
                                 "        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,\n"
                                 "        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000\n"
                                 "};\n"
                                 "\n"
                                 "uint modesChecksum(unsigned char *msg, int bits) {\n"
                                 "    uint crc = 0;\n"
                                 "    int offset = (bits == 112) ? 0 : (112-56);\n"
                                 "    int j;\n"
                                 "\n"
                                 "    for(j = 0; j < bits; j++) {\n"
                                 "        int byte = j/8;\n"
                                 "        int bit = j%8;\n"
                                 "        int bitmask = 1 << (7-bit);\n"
                                 "\n"
                                 "        /* If bit is set, xor with corresponding table entry. */\n"
                                 "        if (msg[byte] & bitmask)\n"
                                 "            crc ^= modes_checksum_table[j+offset];\n"
                                 "    }\n"
                                 "    return crc; /* 24 bit checksum. */\n"
                                 "}\n"
                                 "\n"
                                 "\n"
                                 "__constant int masks[16] = {0b10000000,\n"
                                 "                            0b01000000,\n"
                                 "                            0b00100000,\n"
                                 "                            0b00010000,\n"
                                 "                            0b00001000,\n"
                                 "                            0b00000100,\n"
                                 "                            0b00000010,\n"
                                 "                            0b00000001};\n"
                                 "/* Similar to fixSingleBitErrors() but try every possible two bit combination.\n"
                                 " * This is very slow and should be tried only against DF17 messages that\n"
                                 " * don't pass the checksum, and only in Aggressive Mode.\n"
                                 " * fixSingleBitErrors has already been called if this is run. Don't bother just flipping 1 bit.\n"
                                 " *  */\n"
                                 "__kernel void fixTwoBitsErrors(__global unsigned char *msg,__global int *bits, __global int *returnValue) {\n"
                                 "    unsigned char aux[112/8];\n"
                                 "    uint j = get_global_id(0);\n"
                                 "    uint i = get_global_id(0);\n"
                                 "\n"
                                 "    int byte1 = j/8;\n"
                                 "    int bitmask1 = masks[j%8];//1 << (7-(j%8));\n"
                                 "    /* Don't check the same pairs multiple times, so i starts from j+1 */\n"
                                 "    for (int i = j + 1; i < *bits; i++){\n"
                                 "        int byte2 = i/8;\n"
                                 "        int bitmask2 = masks[i%8];//1 << (7-(i%8));\n"
                                 "        uint crc1, crc2;\n"
                                 "\n"
                                 "        for (int k = 0; k < (*bits)/8; k++){\n"
                                 "            aux[k] = msg[k];\n"
                                 "        }\n"
                                 "\n"
                                 "        aux[byte1] ^= bitmask1; /* Flip j-th bit. */\n"
                                 "        aux[byte2] ^= bitmask2; /* Flip i-th bit. */\n"
                                 "\n"
                                 "        crc1 = ((uint)aux[((*bits)/8)-3] << 16) |\n"
                                 "               ((uint)aux[((*bits)/8)-2] << 8) |\n"
                                 "               (uint)aux[((*bits)/8)-1];\n"
                                 "        crc2 = modesChecksum(aux,*bits);\n"
                                 "\n"
                                 "        if (crc1 == crc2) {\n"
                                 "            /* The error is fixed. Overwrite the original buffer with\n"
                                 "             * the corrected sequence, and returns the error bit\n"
                                 "             * position. */\n"
                                 "            for (int k = 0; k < (*bits)/8; k++){\n"
                                 "                msg[k] = aux[k];\n"
                                 "            }\n"
                                 "\n"
                                 "            /* We return the two bits as a 16 bit integer by shifting\n"
                                 "             * 'i' on the left. This is possible since 'i' will always\n"
                                 "             * be non-zero because i starts from j+1. */\n"
                                 "            *returnValue =  j | (i<<8);\n"
                                 "        }\n"
                                 "    }\n"
                                 "}\n"
                                 "";

    const char *modesChecksumCode = "\n"
                                    "__constant int modes_checksum_table[112] = {\n"
                                    "        0x3935ea, 0x1c9af5, 0xf1b77e, 0x78dbbf, 0xc397db, 0x9e31e9, 0xb0e2f0, 0x587178,\n"
                                    "        0x2c38bc, 0x161c5e, 0x0b0e2f, 0xfa7d13, 0x82c48d, 0xbe9842, 0x5f4c21, 0xd05c14,\n"
                                    "        0x682e0a, 0x341705, 0xe5f186, 0x72f8c3, 0xc68665, 0x9cb936, 0x4e5c9b, 0xd8d449,\n"
                                    "        0x939020, 0x49c810, 0x24e408, 0x127204, 0x093902, 0x049c81, 0xfdb444, 0x7eda22,\n"
                                    "        0x3f6d11, 0xe04c8c, 0x702646, 0x381323, 0xe3f395, 0x8e03ce, 0x4701e7, 0xdc7af7,\n"
                                    "        0x91c77f, 0xb719bb, 0xa476d9, 0xadc168, 0x56e0b4, 0x2b705a, 0x15b82d, 0xf52612,\n"
                                    "        0x7a9309, 0xc2b380, 0x6159c0, 0x30ace0, 0x185670, 0x0c2b38, 0x06159c, 0x030ace,\n"
                                    "        0x018567, 0xff38b7, 0x80665f, 0xbfc92b, 0xa01e91, 0xaff54c, 0x57faa6, 0x2bfd53,\n"
                                    "        0xea04ad, 0x8af852, 0x457c29, 0xdd4410, 0x6ea208, 0x375104, 0x1ba882, 0x0dd441,\n"
                                    "        0xf91024, 0x7c8812, 0x3e4409, 0xe0d800, 0x706c00, 0x383600, 0x1c1b00, 0x0e0d80,\n"
                                    "        0x0706c0, 0x038360, 0x01c1b0, 0x00e0d8, 0x00706c, 0x003836, 0x001c1b, 0xfff409,\n"
                                    "        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,\n"
                                    "        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,\n"
                                    "        0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000\n"
                                    "};\n"
                                    "\n"
                                    "__constant bitmasks[] = {\n"
                                    "        0b10000000, 0b1000000, 0b100000, 0b10000, 0b1000, 0b100, 0b10, 0b1\n"
                                    "};\n"
                                    "\n"
                                    "__kernel void modesChecksum(__local unsigned char **msg, __local int* bits, __global int* crc) {\n"
                                    "    *crc = 0;\n"
                                    "    int offset = (*bits == 112) ? 0 : (112-56);\n"
                                    "    int j = get_global_id(0);\n"
                                    "    int byte = j/8;\n"
                                    "    int bit = j%8;\n"
                                    "    int bitmask = bitmasks[bit];\n"
                                    "\n"
                                    "    /* If bit is set, xor with corresponding table entry. */\n"
                                    "    if (msg[j][byte] & bitmask)\n"
                                    "        *crc ^= modes_checksum_table[j+offset]; /* 24 bit checksum. */\n"
                                    "}";


    g_OpenCL_Dat.singleBit = clCreateProgramWithSource(g_OpenCL_Dat.context, 1, (const char **) &SingleBitErrorCode, NULL, err);
    if (err[0] != CL_SUCCESS){ // I'd really like to merge this and the check below, but I do not know if releasing unallocated memory is defined.
        // TODO: Error checking
        free_resources();
        return err[0];
    }

    g_OpenCL_Dat.doublebit = clCreateProgramWithSource(g_OpenCL_Dat.context, 1, (const char**) &TwoBitErrorCode, NULL, err);
    if (err[0] != CL_SUCCESS){
        // TODO: Error checking
        free_resources();
        return err[0];
    }

    g_OpenCL_Dat.modesChecksum = clCreateProgramWithSource(g_OpenCL_Dat.context, 1, (const char**) &modesChecksumCode, NULL, err);
    if (err[0] != CL_SUCCESS){
        // TODO: Error checking
        free_resources();
        return err[0];
    }

    char buildoptions[] = OPENCL_BUILD_FLAGS;
    err[0] = clBuildProgram(g_OpenCL_Dat.singleBit, 0, NULL, buildoptions, NULL, NULL);

    if (err[0] != CL_SUCCESS){ // Print the error code
        fprintf(stderr, "Error building program\n"); // TODO: Encapsulate this into a function
        perror("1\n");
        char buffer[4096];
        size_t length;
        clGetProgramBuildInfo( g_OpenCL_Dat.singleBit, g_OpenCL_Dat.device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &length);
        fprintf(stderr, "%s\n", buffer);
        free_resources();
        return err[0];
    }

    err[0] = clBuildProgram(g_OpenCL_Dat.doublebit, 0, NULL, buildoptions, NULL, NULL);

    if (err[0] != CL_SUCCESS){ // Print the error code
        perror("2\n");
        fprintf(stderr, "Error building program\n"); // TODO: Encapsulate this into a function
        char buffer[4096];
        size_t length;
        clGetProgramBuildInfo( g_OpenCL_Dat.doublebit, g_OpenCL_Dat.device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &length);
        fprintf(stderr, "%s\n", buffer);
        free_resources();
        return err[0];
    }

    err[0] = clBuildProgram(g_OpenCL_Dat.modesChecksum, 0, NULL, buildoptions, NULL, NULL);

    if (err[0] != CL_SUCCESS){ // Print the error code
        perror("3\n");
        fprintf(stderr, "Error building program\n"); // TODO: Encapsulate this into a function
        char buffer[4096];
        size_t length;
        clGetProgramBuildInfo( g_OpenCL_Dat.modesChecksum, g_OpenCL_Dat.device, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &length);
        fprintf(stderr, "%s\n", buffer);
        free_resources();
        return err[0];
    }


    // Creating the kernels

    g_OpenCL_Dat.k_singleBit = clCreateKernel(g_OpenCL_Dat.singleBit, "fixSingleBitErrors", err);
    g_OpenCL_Dat.k_doubleBit = clCreateKernel(g_OpenCL_Dat.doublebit, "fixTwoBitsErrors", err+1);
    g_OpenCL_Dat.k_modesChecksum = clCreateKernel(g_OpenCL_Dat.modesChecksum, "modesChecksum", err+2);
    for (int i = 0; i < 3; i++){
        if (err[i] != CL_SUCCESS){
            fprintf(stderr, "%d\n", i);
            Error_Kernel(err[i]);
            return err[i];
        }
    }


    // Creating arguments

    g_OpenCL_Dat.bitf_output = clCreateBuffer(g_OpenCL_Dat.context, CL_MEM_WRITE_ONLY, sizeof(cl_int) * MODES_LONG_MSG_BITS, NULL, err);
    g_OpenCL_Dat.bits = clCreateBuffer(g_OpenCL_Dat.context, CL_MEM_READ_ONLY, sizeof(int), NULL, err + 1); // int, not cl_int because thats what is input into our errorfix functions for now
    g_OpenCL_Dat.bitf_message = clCreateBuffer(g_OpenCL_Dat.context, CL_MEM_READ_WRITE, sizeof(unsigned char) * MODES_LONG_MSG_BYTES, NULL, err + 2);
    g_OpenCL_Dat.modes_msgs_buf = clCreateBuffer(g_OpenCL_Dat.context, CL_MEM_READ_WRITE, sizeof(char)*MODES_LONG_MSG_BITS*MODES_LONG_MSG_BITS, NULL, err+3);
    g_OpenCL_Dat.modes_crcs = clCreateBuffer(g_OpenCL_Dat.context, CL_MEM_READ_WRITE, sizeof(cl_int)*MODES_LONG_MSG_BITS*MODES_LONG_MSG_BITS, NULL, err+4);
    for (int i = 0; i < 5; i++){
        if (err[i] != CL_SUCCESS){
            checkCreateArgBuf(err[i]);
            return -2;
        }
    }


    // We are finished!
    return 0;
}

#endif //DUMP1090_GPUPREPARATIONS_H
