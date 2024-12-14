#include <wind/userface/userf.h>

#include <iostream>
#include <assert.h>

int main(int argc, char **argv) {
  WindUserInterface *ui = new WindUserInterface(argc, argv);
  ui->processFiles();
  return 0;
}
