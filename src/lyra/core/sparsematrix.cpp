#include "sparsematrix.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>

#include "tools.hpp"

Triplet::Triplet (ul_t _i, ul_t _j, real_t _value) : i (_i),
                                                     j (_j),
                                                     value (_value)
{
}

SparseMatrix::SparseMatrix () : m_rows ({}),
                                m_np (0)
{
}

SparseMatrix::~SparseMatrix ()
{
    Clear ();
}

void
SparseMatrix::Init (ul_t numOfPoints)
{
    this->Clear ();
    m_np = numOfPoints;

    for (ul_t id = 0; id < m_np; ++id)
        m_rows.push_back (new RowCompactor ());

    return;
}

void
SparseMatrix::Clear ()
{
    m_np = 0;
    for (RowCompactor * row : m_rows)
        delete row;
    m_rows.clear ();
    return;
}

ul_t
SparseMatrix::NonZeros ()
{
    ul_t out = 0;
    for (RowCompactor * row : m_rows)
        out += row->value.size ();

    return out;
}

void
SparseMatrix::PrintSparseView (std::ostream & stream)
{
    RowCompactor * row = nullptr;

    stream << "NNZ : " << NonZeros () << std::endl;
    for (ul_t rowid = 0; rowid < m_np; ++rowid)
    {
        row = m_rows [rowid];

        stream << "i : " << rowid << std::flush;
        for (ul_t id = 0; id < row->colid.size (); ++id)
            stream << " [j : " << row->colid [id] << ", value : " << row->value [id] << "] \t";
        stream << std::endl;
    }

    return;
}

void
SparseMatrix::PrintDenseView (std::ostream & stream)
{
    RowCompactor * row;
    ul_t           colcursor, colidsize;

    stream << "NNZ : " << NonZeros () << std::endl;
    for (ul_t rowid = 0; rowid < m_np; ++rowid)
    {
        row       = m_rows [rowid];
        colcursor = 0;
        colidsize = row->colid.size ();

        for (ul_t colid = 0; colid < m_np || colcursor < colidsize; ++colid)
        {
            if ((colcursor < colidsize) && (row->colid [colcursor] == colid))
            {
                stream << std::setw (8) << row->value [colcursor] << std::flush;
                colcursor++;
            }
            else
            {
                stream << std::setw (8) << 0.0 << std::flush;
            }
        }
        stream << std::endl;
    }

    return;
}

std::vector<real_t>
SparseMatrix::operator* (const std::vector<real_t> & vec) const
{
    std::vector<real_t> out (vec.size ());

    if (vec.size () != m_np)
        return out;

    RowCompactor * row  = nullptr;
    real_t *       data = nullptr;
    for (ul_t rowid = 0; rowid < m_np; ++rowid)
    {
        row  = m_rows [rowid];
        data = &out [rowid];

        *data = 0.0;

        for (ul_t id = 0; id < row->colid.size (); ++id)
            *data += row->value [id] * vec [row->colid [id]];
    }

    return out;
}

real_t
SparseMatrix::RowProduct (const std::vector<real_t> & vec, ul_t rowid) const
{
    RowCompactor * row  = m_rows [rowid];
    real_t         data = 0;
    ul_t           size = row->colid.size ();

    // #pragma code_align 32
    for (ul_t id = 0; id < size; ++id)
        data += row->value [id] * vec [row->colid [id]];

    return data;
}

void
SparseMatrix::SetFromTriplet (typename TripletsList::iterator beg,
                              typename TripletsList::iterator end)
{
    bool           find = false;
    RowCompactor * row  = nullptr;

    for (typename TripletsList::iterator it = beg; it < end; ++it)
    {
        if (it->i >= m_np)
            continue;

        find = false;
        row  = m_rows [it->i];

        for (ul_t id = 0; id < row->colid.size (); ++id)
            if (row->colid [id] == it->j)
            {
                row->value [id] += it->value;
                find = true;
            }

        if (find || it->j >= m_np)
            continue;

        row->colid.push_back (it->j);
        row->value.push_back (it->value);
    }

    for (RowCompactor * row : m_rows)
    {
        auto compare = [row] (const ul_t & i, const ul_t & j) -> bool {
            return row->colid [i] < row->colid [j];
        };

        std::vector<ul_t> indices (row->colid.size ());
        std::iota (indices.begin (), indices.end (), 0);

        std::sort (indices.begin (), indices.end (), compare);

        apply_permutation (row->value, indices);
        apply_permutation (row->colid, indices);
    }

    return;
}

void
SparseMatrix::Pruned (real_t epsilon)
{
    for (RowCompactor * row : m_rows)
        for (ul_t id = 0; id < row->colid.size (); ++id)
            if (std::abs (row->value [id]) < epsilon)
            {
                row->value.erase (row->value.begin () + id);
                row->colid.erase (row->colid.begin () + id);
            }

    return;
}