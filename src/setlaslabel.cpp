/*
===============================================================================

  FILE:  setlaslabel.cpp
  use the lastools to change a point's classification: https://groups.google.com/g/lastools/c/2DSilXCTlxQ
  
  CONTENTS:
  
    This source code serves as an example how you can easily use LASlib to
    write your own processing tools or how to import from and export to the
    LAS format or - its compressed, but identical twin - the LAZ format.

  PROGRAMMERS:

    info@rapidlasso.de  -  https://rapidlasso.de

  COPYRIGHT:

    (c) 2007-2014, rapidlasso GmbH - fast tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the LICENSE.txt file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    3 January 2011 -- created while too homesick to go to Salzburg with Silke
  
===============================================================================
*/


#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <fstream>
#include <vector>

#include "data/label_utils.h"
#include "lasreader.hpp"
#include "laswriter.hpp"

void usage(bool wait=false)
{
  fprintf(stderr,"usage:\n");
  fprintf(stderr,"lasexample in.las in.label out.las\n");
  fprintf(stderr,"lasexample -i in.las -l in.label -o out.las -verbose\n");
  fprintf(stderr,"lasexample -h\n");
  if (wait)
  {
    fprintf(stderr,"<press ENTER>\n");
    getc(stdin);
  }
  exit(1);
}

static void byebye(bool error=false, bool wait=false)
{
  if (wait)
  {
    fprintf(stderr,"<press ENTER>\n");
    getc(stdin);
  }
  exit(error);
}

static double taketime()
{
  return (double)(clock())/CLOCKS_PER_SEC;
}

void readLabels(const std::string& filename, std::vector<uint32_t>& labels) {
  std::ifstream in(filename.c_str(), std::ios::binary);
  if (!in.is_open()) {
    std::cerr << "Unable to open label file. " << std::endl;
    return;
  }

  labels.clear();

  in.seekg(0, std::ios::end);
  uint32_t num_points = in.tellg() / (sizeof(uint32_t));
  in.seekg(0, std::ios::beg);

  labels.resize(num_points);
  in.read((char*)&labels[0], num_points * sizeof(uint32_t));

  in.close();
}

void remapLabels(std::vector<uint32_t>& labels, const std::map<uint32_t, uint32_t>& remap) {
  for (uint32_t i = 0; i < labels.size(); ++i) {
    const auto it = remap.find(labels[i]);
    if (it != remap.end()) {
      labels[i] = it->second;
    } else {
      labels[i] = 0;
      std::cerr << "Label " << labels[i] << " not found in remap. " << std::endl;
    }
  }
}

std::map<std::string, uint32_t> semReduced{
  {"unlabeled", 0},
  {"agent", 11},
  {"ground", 2},
  {"vegetation", 4},
  {"structure", 6},
};

std::map<std::string, std::string> semKitti2semReduced{
{"unlabeled", "unlabeled"},
{"motorcycle", "agent"},
{"person", "agent"},
{"car", "agent"},
{"road", "ground"},
{"other-ground", "ground"}, // other ground may include some other-structure.
{"vegetation", "vegetation"},
{"trunk", "vegetation"}, // trunk includes trees.
{"pole", "structure"}, 
{"other-structure", "structure"}, // other structure includes lamp and levee.
{"building", "structure"},
{"fence", "structure"},
};

void constructSemReduced2SemKitti(const std::string &labelxml, std::map<uint32_t, uint32_t> &semKittiId2semReducedId) {
  std::map<uint32_t, std::string> label_names;
  getLabelNames(labelxml, label_names);
  std::map<std::string, uint32_t> semKitti;
  for (auto it = label_names.begin(); it != label_names.end(); ++it) {
    semKitti[it->second] = it->first;
  }
  for (auto it = semKitti2semReduced.begin(); it != semKitti2semReduced.end(); ++it) {
    semKittiId2semReducedId[semKitti[it->first]] = semReduced[it->second];
  }
}

int main(int argc, char *argv[])
{
  int i;
  bool verbose = false;
  double start_time = 0.0;
  std::string labelfile;
  std::string inlasfile;
  std::string outlasfile;
  LASreadOpener lasreadopener;
  LASwriteOpener laswriteopener;

  if (argc == 1)
  {
    usage();
  }
  else
  {
    lasreadopener.parse(argc, argv);
    laswriteopener.parse(argc, argv);
  }

  for (i = 1; i < argc; i++)
  {
    if (argv[i][0] == '\0')
    {
      continue;
    }
    else if (strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"-help") == 0)
    {
      usage();
    }
    else if (strcmp(argv[i],"-v") == 0 || strcmp(argv[i],"-verbose") == 0)
    {
      verbose = true;
    }
    else if (i == argc - 3 && !lasreadopener.active() && !laswriteopener.active())
    {
      lasreadopener.set_file_name(argv[i]);
      inlasfile = argv[i];
    } else if (i == argc - 2) {
      labelfile = argv[i];
    }
    else if (i == argc - 1 && !lasreadopener.active() && !laswriteopener.active())
    {
      lasreadopener.set_file_name(argv[i]);
      inlasfile = argv[i];
    }
    else if (i == argc - 1 && lasreadopener.active() && !laswriteopener.active())
    {
      laswriteopener.set_file_name(argv[i]);
      outlasfile = argv[i];
    }
    else
    {
      fprintf(stderr, "ERROR: cannot understand argument '%s'\n", argv[i]);
      usage();
    }
  }

  if (verbose) start_time = taketime();

  // check input & output

  if (!lasreadopener.active())
  {
    fprintf(stderr,"ERROR: no input specified\n");
    usage(argc == 1);
  }

  if (!laswriteopener.active())
  {
    fprintf(stderr,"ERROR: no output specified\n");
    usage(argc == 1);
  }

  // open lasreader

  LASreader* lasreader = lasreadopener.open();
  if (lasreader == 0)
  {
    fprintf(stderr, "ERROR: could not open lasreader\n");
    byebye(argc==1);
  }

  // open laswriter

  LASwriter* laswriter = laswriteopener.open(&lasreader->header);
  if (laswriter == 0)
  {
    fprintf(stderr, "ERROR: could not open laswriter\n");
    byebye(argc==1);
  }

#ifdef _WIN32
  if (verbose) fprintf(stderr, "reading %I64d points from '%s' and writing them modified to '%s'.\n", lasreader->npoints, lasreadopener.get_file_name(), laswriteopener.get_file_name());
#else
  if (verbose) fprintf(stderr, "reading %lld points from '%s' and writing them modified to '%s'.\n", lasreader->npoints, lasreadopener.get_file_name(), laswriteopener.get_file_name());
#endif

  // loop over points and modify them
  int64_t num_points = lasreader->npoints;
  std::cout << "Load " << num_points << " points from " << inlasfile << std::endl;
  std::vector<uint32_t> labels;
  readLabels(labelfile, labels);
  std::cout << "Load " << labels.size() << " labels from " << labelfile << std::endl;
  if (labels.size() != (size_t)num_points) 
  {
    std::cerr << "Number of labels does not match number of points!" << std::endl;
    return 1;
  }

  std::map<uint32_t, uint32_t> semKittiId2semReducedId;
  constructSemReduced2SemKitti("labels.xml", semKittiId2semReducedId);
  remapLabels(labels, semKittiId2semReducedId);

  // where there is a point to read
  size_t idx = 0;
  while (lasreader->read_point())
  {
    // modify the point
    // uint16_t psid = lasreader->point.get_point_source_ID();
    // double user_data = lasreader->point.get_user_data();
    uint32_t label = labels[idx];
    uint8_t c = lasreader->point.get_classification();
    if (c == 0) lasreader->point.set_classification(label);
    else std::cout << "Point already labeled to " << c << std::endl;
    // write the modified point
    laswriter->write_point(&lasreader->point);
    // add it to the inventory
    laswriter->update_inventory(&lasreader->point);
    idx++;
  }
  if (idx != (size_t)num_points) 
  {
    std::cerr << "Number of points read does not match number of points!" << std::endl;
    return 1;
  }

  laswriter->update_header(&lasreader->header, TRUE);

  I64 total_bytes = laswriter->close();
  delete laswriter;

#ifdef _WIN32
  if (verbose) fprintf(stderr,"total time: %g sec %I64d bytes for %I64d points\n", taketime()-start_time, total_bytes, lasreader->p_count);
#else
  if (verbose) fprintf(stderr,"total time: %g sec %lld bytes for %lld points\n", taketime()-start_time, total_bytes, lasreader->p_count);
#endif

  lasreader->close();
  delete lasreader;

  return 0;
}
