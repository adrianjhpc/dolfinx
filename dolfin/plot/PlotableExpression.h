// Copyright (C) 2012 Fredrik Valdmanis
//
// This file is part of DOLFIN.
//
// DOLFIN is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// DOLFIN is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with DOLFIN. If not, see <http://www.gnu.org/licenses/>.
//
// First added:  2012-06-02
// Last changed: 2012-06-02

#ifndef __PLOTABLE_EXPRESSION_H
#define __PLOTABLE_EXPRESSION_H

#include <dolfin/mesh/Mesh.h>
#include <dolfin/function/Expression.h>

namespace dolfin
{

  /// A light wrapper class to hold an expression to plot, along with the mesh
  /// to plot it on. Allows for clean, templated plotter code in plot.cpp
  class PlotableExpression
  {
  public:

    /// Create plotable expression object
    explicit PlotableExpression(const Expression& expression, const Mesh& mesh);

    /// Return unique ID of the expression
    uint id() const { return _expression->id(); }

    /// Get the expression
    boost::shared_ptr<const Expression> expression() const
    { return _expression; }

    /// Get the mesh
    boost::shared_ptr<const Mesh> mesh() const
    { return _mesh; }

  private:

    // The mesh to plot on
    boost::shared_ptr<const Mesh> _mesh;

    // The expression itself
    boost::shared_ptr<const Expression> _expression;

  };

}

#endif
