// ===================== begin_copyright_notice ============================
//
// Copyright (C) 2023-2024 Intel Corporation
//
// SPDX-License-Identifier: MIT
//
// ===================== end_copyright_notice ==============================

/**
 * @file   statistics.h
 *
 * @brief Declaration of GITS file statistics.
 *
 */

#pragma once

#include "library.h"
#include "runner.h"
#include <map>
#include <set>
#include <string>

namespace gits {
class CScheduler;
class CToken;
class CStatistics;

class CStatsComputer : public CAction {
  CStatistics& _stats;

public:
  CStatsComputer(gits::CStatistics& stats) : _stats(stats) {}
  void Run(CToken& token) override;
};

template <size_t NUMCOLS>
class CAsciiTable {
  // There are normal rows with NUMCOLS cells and special rows with only one
  // cell, so we have to use a a vector to hold both sizes. A better solution
  // would be a tagged union like C++17's std::variant<string, array<string>>.
  typedef std::vector<std::string> Row;

  // A first row of the table; contains column names.
  std::array<std::string, NUMCOLS> columnHeaders;
  std::vector<Row> rows;
  // Not counting the 1-space padding on both sides of each column.
  std::array<size_t, NUMCOLS> columnWidths{};
  const static char v = '|', h = '=';

public:
  // Accepting only a fixed-size array to avoid length checking.
  CAsciiTable(std::array<std::string, NUMCOLS>& headers) {
    columnHeaders = std::move(headers);
    for (size_t col = 0; col < NUMCOLS; ++col) {
      columnWidths[col] = columnHeaders[col].length();
    }
  }

  void AddRow(std::array<std::string, NUMCOLS>& row) {
    for (size_t col = 0; col < NUMCOLS; ++col) {
      columnWidths[col] = std::max(columnWidths[col], row[col].length());
    }
    const Row properRow(row.begin(), row.end());
    rows.push_back(properRow);
  }

  void AddOneCellRow(std::string cellContents) {
    const Row properRow{std::move(cellContents)};
    rows.push_back(properRow);
  }

  void Print(std::ostream& out) const {
    size_t summedWidth = 0; // Width from left '|' to right '|' (excluding the '|'s).
    for (const size_t col : columnWidths) {
      summedWidth += col + 2; // +2 because of 1-space padding on both sides.
    }
    summedWidth += columnWidths.size() - 1; // Because of the '|' chars between columns.

    printHorizLine(out, summedWidth);
    const Row headersVec(columnHeaders.begin(), columnHeaders.end());
    printRow(out, headersVec, true);
    printHorizLine(out, summedWidth);
    for (const auto& row : rows) {
      if (row.size() == 1) {
        printSingleItemRow(out, row[0], summedWidth);
      } else if (row.size() == NUMCOLS) {
        printRow(out, row, false);
      } else {
        assert("Row has wrong size, logic error." && 0);
      }
    }
    printHorizLine(out, summedWidth, ' ');
    printHorizLine(out, summedWidth);
  }

private:
  // Print a line of a given width (width not counting vertical frame elements).
  void printHorizLine(std::ostream& out, size_t width, char c = h) const {
    out << v;
    for (size_t i = 0; i < width; ++i) {
      out << c;
    }
    out << v << std::endl;
  }

  // Print one cell of given width (width not counting 1-space padding on both sides).
  void printCell(std::ostream& out,
                 std::string contents,
                 size_t width,
                 bool leftAligned = false) const {
    if (contents.length() > width) {
      contents = contents.substr(0, width);
    }
    out << ' ';
    if (leftAligned) {
      out << contents;
      for (size_t i = 0; i < width - contents.length(); ++i) {
        out << ' ';
      }
    } else {
      for (size_t i = 0; i < width - contents.length(); ++i) {
        out << ' ';
      }
      out << contents;
    }
    out << ' ';
  }

  // Print a normal row with NUMCOLS cells.
  void printRow(std::ostream& out, Row row, bool allLeftAligned) const {
    assert(row.size() == NUMCOLS);
    for (size_t col = 0; col < row.size(); ++col) {
      out << v;
      bool leftAligned;
      if (allLeftAligned) { // Because headers row needs to be left-aligned.
        leftAligned = true;
      } else { // Other rows have 1st column aligned left, but other columns aligned right.
        leftAligned = col == 0 ? true : false;
      }
      printCell(out, row[col], columnWidths[col], leftAligned);
    }
    out << v << std::endl;
  }

  // Print a row consisting of 1 cell that spans the entire table width.
  void printSingleItemRow(std::ostream& out, std::string row, size_t totalWidth) const {
    // Because printCell will add 1-space padding on both sides.
    const size_t rowWidth = totalWidth - 2;
    if (row.length() > rowWidth) {
      row = row.substr(0, rowWidth);
    }

    out << v;
    const bool leftAligned = true;
    printCell(out, std::move(row), rowWidth, leftAligned);
    out << v << std::endl;
  }
};

class CStatistics {
  struct TCall {
    unsigned num;
    unsigned numPerFrameMin;
    unsigned numPerFrameMax;
    unsigned framesNum;

    unsigned currFrame;
    unsigned currFrameNum;

    bool skipped;

    TCall();
    void NewFrame();
  };

  typedef std::map<std::string, TCall> CCallStats;
  typedef std::map<CLibrary::TId, CCallStats> CLibraryStats;

  typedef std::set<unsigned> CVersionIds;
  typedef std::set<unsigned> CCallsIds;

  bool _init;
  unsigned _framesNum;
  unsigned long _callsNum;
  unsigned long _initCallsNum;
  unsigned long _appCallsNum;

  CLibraryStats _libraryStats;
  CVersionIds _versionIds;
  CCallsIds _callsIds;

public:
  CStatistics();
  void Get(CScheduler& scheduler, CStatsComputer& comp);
  void AddToken(const gits::CToken& token);
  void Print() const;
};
} // namespace gits
