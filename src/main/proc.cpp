/**
 * @file proc.cpp
 * @brief Entry point for the Wind compiler.
 */

#include <wind/userface/userf.h>
#include <wind/isc/isc.h>

#include <iostream>
#include <assert.h>

#include <wind/backend/writer/writer.h>
#include <wind/generation/IR.h>
#include <wind/backend/x86_64/backend.h>

/**
 * @brief Main function for the Wind compiler.
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return Exit status.
 */
int main(int argc, char **argv) {
  InitISC();
  WindUserInterface *ui = new WindUserInterface(argc, argv);
  ui->processFiles();
  delete ui;
  return 0;
}