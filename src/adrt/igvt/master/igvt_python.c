#include "igvt_python.h"
#include "Python.h"


char *igvt_python_response;


void igvt_python_init(void);
void igvt_python_command(char *command);
static PyObject* igvt_python_stdout(PyObject *self, PyObject* args);


static PyMethodDef IGVT_Methods[] = {
    {"stdout", igvt_python_stdout, METH_VARARGS, "redirected output."},
#if 0
    {"camera.set.position", igvt_python_set_camera_position, METH_VARARGS, "set camera position."},
    {"camera.get.position", igvt_python_get_camera_position, METH_VARARGS, "get camera position."},
#endif
    {NULL, NULL, 0, NULL}
};


void igvt_python_init() {
  Py_Initialize();


  igvt_python_response = (char *)malloc(1024);

  PyImport_AddModule("adrt");
  Py_InitModule("adrt", IGVT_Methods);
  PyRun_SimpleString("import adrt");
  

  /* Redirect the output */
  PyRun_SimpleString("\
import sys\n\
import string\n\
class Redirect:\n\
    def __init__(self, stdout):\n\
        self.stdout = stdout\n\
    def write(self, s):\n\
        adrt.stdout(s)\n\
sys.stdout = Redirect(sys.stdout)\n\
sys.stderr = Redirect(sys.stderr)\n\
");

}


void igvt_python_free() {
  free(igvt_python_response);
  Py_Finalize();
}


void igvt_python_code(char *code) {
  igvt_python_response[0] = 0;
  PyRun_SimpleString(code);
  strcpy(code, igvt_python_response);
}


/* Called once for every line */
static PyObject* igvt_python_stdout(PyObject *self, PyObject* args) {
  char *string;

  if(PyArg_ParseTuple(args, "s", &string))
    strcat(igvt_python_response, string);

  return PyInt_FromLong(42L);
}
