#include <wind/userface/userf.h>
#include <wind/isc/isc.h>

#include <iostream>
#include <assert.h>

#include <wind/backend/writer/writer.h>
int main(int argc, char **argv) {
  if (argc > 1) {
    InitISC();
    WindUserInterface *ui = new WindUserInterface(argc, argv);
    ui->processFiles();
    delete ui;
    return 0;
  }
  Ax86_64 *x86asm = new Ax86_64();
  x86asm->BindSection(x86asm->NewSection(".text"));
  x86asm->BindLabel(x86asm->NewLabel("main"));
  
  std::cerr << x86asm->Emit() << std::endl;
}