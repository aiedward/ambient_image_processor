/*
    Ambient Image Processor - A tool to perform several imaging tasks
    
    Copyright (C) 2016 Josef Koller

    https://github.com/josefkoller/ambient_image_processor    
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <iostream>
#include <string>
typedef std::string String;

void perform(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  //  std::cout << "started program: " << argv[0] << std::endl;
    if(argc > 1)
    {
 //       std::cout << "database file path: " << argv[1] << std::endl;
    }
    else
    {
        std::cout << "Usage: database_file_path [entropy_kde_kernel_bandwidth]" << std::endl;
        return 1;
    }
    perform(argc, argv);
 //   std::cout << "finished program: " << argv[0] << std::endl;
    return 0;
}

typedef const unsigned char* SQLString;
struct RunParameter
{
  int id;
  String denoised_image_path;
  String deshaded_image_path;
  String shading_image_path;
  RunParameter(int id, String denoised_image_path, String deshaded_image_path, String shading_image_path)
    : id(id), denoised_image_path(denoised_image_path), deshaded_image_path(deshaded_image_path), shading_image_path(shading_image_path) {}
};

#include <vector>
#include <sqlite3.h>

typedef sqlite3* Database;


Database openDatabase(const char* database_file_path)
{
  sqlite3* database;
  if(sqlite3_open(database_file_path, &database) != SQLITE_OK)
  {
    std::cout << "SQLite ERROR: " << sqlite3_errmsg(database) << std::endl;
    return nullptr;
  }
  return database;
}

std::vector<RunParameter> readRunParameters(Database database)
{
  std::vector<RunParameter> run_parameters;
  std::string query = "select id, output_denoised_file, output_deshaded_file, output_shading_file from run";
  sqlite3_stmt* statement;
  sqlite3_prepare(database, query.c_str(), -1, &statement, NULL);
  while(sqlite3_step(statement) != SQLITE_DONE)
  {
    int id = sqlite3_column_int(statement, 0);
    SQLString denoised_image_path = sqlite3_column_text(statement, 1);
    SQLString deshaded_image_path = sqlite3_column_text(statement, 2);
    SQLString shading_image_path = sqlite3_column_text(statement, 3);
    run_parameters.push_back(RunParameter(
      id,
      reinterpret_cast<const char*>(denoised_image_path),
      reinterpret_cast<const char*>(deshaded_image_path),
      reinterpret_cast<const char*>(shading_image_path) ));
  }
  return run_parameters;
}

struct RunMetrics
{
  float denoised_coefficient_of_variation = 0;
  float denoised_mean_total_variation = 0;
  float deshaded_coefficient_of_variation = 0;
  float deshaded_mean_total_variation = 0;
  float deshaded_entropy = 0;
};

void write_metrics_to_database(RunMetrics run, int run_id, Database database)
{
  char command[512];
  sprintf(command, "update run set denoised_coefficient_of_variation=%f,denoised_mean_total_variation=%f,deshaded_coefficient_of_variation=%f,deshaded_mean_total_variation=%f,deshaded_entropy=%f where id=%d",
  run.denoised_coefficient_of_variation,
  run.denoised_mean_total_variation,
  run.deshaded_coefficient_of_variation,
  run.deshaded_mean_total_variation,
  run.deshaded_entropy,
  run_id);

  sqlite3_stmt* statement;
  if(sqlite3_prepare(database, command, -1, &statement, NULL) != SQLITE_OK)
  {
     std::cout << "  SQL error: " << sqlite3_errmsg(database) << std::endl;
     std::cout << "  SQL command: " << command << std::endl;
     exit(1);
  }
  sqlite3_step(statement);
}

#include "ITKImage.h"
#include "ImageInformationProcessor.h"
#include "CudaImageOperationsProcessor.h"
#include "HistogramProcessor.h"
RunMetrics calculate_metrics(RunParameter parameter, const double kde_bandwidth,
                             const double window_from, const double window_to)
{
  RunMetrics metrics;

  auto denoised_image = ITKImage::read(parameter.denoised_image_path);
  metrics.denoised_mean_total_variation = CudaImageOperationsProcessor::tv(denoised_image) / denoised_image.voxel_count;
  metrics.denoised_coefficient_of_variation = ImageInformationProcessor::coefficient_of_variation(denoised_image);

  auto deshaded_image = ITKImage::read(parameter.deshaded_image_path);
  metrics.deshaded_mean_total_variation = CudaImageOperationsProcessor::tv(deshaded_image) / deshaded_image.voxel_count;
  metrics.deshaded_coefficient_of_variation = ImageInformationProcessor::coefficient_of_variation(deshaded_image);

  ITKImage mask;

  metrics.deshaded_entropy = window_from == window_to ?
              HistogramProcessor::calculateEntropy(deshaded_image, mask, kde_bandwidth) :
              HistogramProcessor::calculateEntropy(deshaded_image, mask, kde_bandwidth,
                                                   window_from, window_to);

  return metrics;
}


void perform(int argc, char *argv[])
{
  const char* database_file_path = argv[1];
  double kde_bandwidth = argc > 2 ? std::stod(argv[2]) : -1;

  double window_from = -1;
  double window_to = -1;
  try
  {
      window_from = argc > 3 ? std::stod(argv[3]) : -1;
      window_to = argc > 4 ? std::stod(argv[4]) : -1;
  }
  catch(std::runtime_error exception)
  {
      std::cerr << exception.what() << std::endl;
  }

  Database database = openDatabase(database_file_path);
  std::vector<RunParameter> run_parameters = readRunParameters(database);
  for(RunParameter parameter : run_parameters)
  {
    RunMetrics metrics = calculate_metrics(parameter, kde_bandwidth, window_from, window_to);
    write_metrics_to_database(metrics, parameter.id, database);
  }
  sqlite3_close(database);
}
