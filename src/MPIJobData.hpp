#pragma once

#include <mpi.h>

struct MPIJobData
{
    char setting[20] = {};
    char originalHash[200] = {};

    char startingPasswd[9] = {};
    char endingPasswd[9] = {};

    static inline MPI::Datatype MPIDataType()
    {
        int sizes[] = {
            sizeof(MPIJobData::setting),
            sizeof(MPIJobData::originalHash),
            sizeof(MPIJobData::startingPasswd),
            sizeof(MPIJobData::endingPasswd),
        };
        MPI::Aint offsets[] = {
            offsetof(MPIJobData, setting),
            offsetof(MPIJobData, originalHash),
            offsetof(MPIJobData, startingPasswd),
            offsetof(MPIJobData, endingPasswd),
        };

        MPI::Datatype datatypes[] = {
            MPI::CHAR,
            MPI::CHAR,
            MPI::CHAR,
            MPI::CHAR,
        };
        auto type = MPI::Datatype::Create_struct(4, sizes, offsets, datatypes);
        type.Commit();
        return type;
    }
};