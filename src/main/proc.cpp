#include <wind/userface/userf.h>
#include <wind/isc/isc.h>

#include <iostream>
#include <assert.h>

int main(int argc, char **argv) {
  InitISC();
  WindUserInterface *ui = new WindUserInterface(argc, argv);
  ui->processFiles();
  return 0;
}
