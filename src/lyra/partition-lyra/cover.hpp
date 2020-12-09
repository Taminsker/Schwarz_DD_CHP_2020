#ifndef SRC_LYRA_PARTITION_LYRA_COVER_HPP
#define SRC_LYRA_PARTITION_LYRA_COVER_HPP

#include "../core/core.hpp"

template <typename T>
error_t
MakeRecoveryZone (Mesh<T> * mesh, ul_t nparts, ul_t nrecover)
{
    ul_t numPoints = mesh->GetNumberOfPoints ();

    for (ul_t idCover = 0; idCover < nrecover; ++idCover)
    {
        std::vector<std::vector<ul_t>> base (numPoints, std::vector<ul_t> ());
        for (ul_t idPoint = 0; idPoint < numPoints; ++idPoint)
            base [idPoint] = mesh->GetPoint (idPoint)->procsidx;

        for (ul_t idProc = 0; idProc < nparts; ++idProc)
        {
            for (ul_t idPoint = 0; idPoint < numPoints; ++idPoint)
            {
                Point<T> * p        = mesh->GetPoint (idPoint);
                bool       imonproc = false;

                for (ul_t idprochere : base [idPoint])
                    if (idprochere == idProc)
                    {
                        imonproc = true;
                        break;
                    }

                if (imonproc)
                {
                    for (ul_t idneighs = 0; idneighs < 4; ++idneighs)
                    {
                        Point<T> * neigh = p->neigh [idneighs];
                        if (neigh)
                        {
                            bool found = neigh->OnTheProc (idProc);

                            if (!found)
                                neigh->procsidx.push_back (idProc);
                        }
                    }
                }
            }
        }
    }

    return EXIT_SUCCESS;
}

template <typename T>
error_t
CheckCover (Mesh<T> * mesh, ul_t nparts)
{
    std::cout << COLOR_BLUE << std::string (60, '-') << ENDLINE;

    ul_t numPoints = mesh->GetNumberOfPoints ();

    for (ul_t idProc = 0; idProc < nparts; ++idProc)
    {
        ul_t cardalone  = 0;
        ul_t cardshared = 0;

        for (ul_t idPoint = 0; idPoint < numPoints; ++idPoint)
        {
            Point<T> * p = mesh->GetPoint (idPoint);

            bool imonproc = false;

            for (ul_t idprochere : p->procsidx)
                if (idprochere == idProc)
                {
                    imonproc = true;
                    break;
                }

            if (imonproc)
            {
                if (p->procsidx.size () == 1)
                    cardalone++;
                else
                    cardshared++;
            }
        }

        if (cardalone < 3)
            WARNING << "the proc " << COLOR_BLUE << idProc << COLOR_YELLOW << " has only " << COLOR_GREEN << cardalone << COLOR_YELLOW << " exclusive points [" << COLOR_RED << cardshared << COLOR_YELLOW << " shared points]..." << ENDLINE;
        else
            INFOS << "the proc " << COLOR_BLUE << idProc << COLOR_DEFAULT << " has " << COLOR_GREEN << cardalone << COLOR_DEFAULT << " exclusive points [" << COLOR_RED << cardshared << COLOR_DEFAULT << " shared points]." << ENDLINE;
    }

    return EXIT_SUCCESS;
}
#endif /* SRC_LYRA_PARTITION_LYRA_COVER_HPP */
