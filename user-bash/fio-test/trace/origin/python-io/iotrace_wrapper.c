#include <Python.h>


PyObject* wrap_initialize(PyObject* self, PyObject* args) 
{
	  int result;
	  char *filename;
	  
	    
    if (! PyArg_ParseTuple(args, "s:initialize",  &filename))
		    return NULL;
	  
	  result = initialize(filename);
	  return Py_BuildValue("i", result);
}


PyObject* wrap_nextrequest(PyObject* self, PyObject* args) 
{
	  short devno;
	  unsigned long startblk;
	  int bytecount;
	  char rwType;
	  double reqtime;
	    
	  int result;
	  PyObject* pTuple;  
	  result = nextrequest(&devno, &startblk, &bytecount,&rwType,&reqtime);
		// create the tuple
		pTuple = PyTuple_New(5);
		assert(PyTuple_Check(pTuple));
		assert(PyTuple_Size(pTuple) == 5);
		
		// set the item
		PyTuple_SetItem(pTuple, 0, Py_BuildValue("h", devno));
		PyTuple_SetItem(pTuple, 1, Py_BuildValue("k", startblk));
		PyTuple_SetItem(pTuple, 2, Py_BuildValue("i", bytecount));
	  PyTuple_SetItem(pTuple, 3, Py_BuildValue("c", rwType));
	  PyTuple_SetItem(pTuple, 4, Py_BuildValue("d", reqtime));
	  return pTuple;
}



PyObject* wrap_finalize(PyObject* self, PyObject* args) 
{
	  int result;
	    
    result = finalize();
	  return Py_BuildValue("i", result);
}

static PyMethodDef IotraceMethods[] = 
{
	  {"initialize", wrap_initialize, METH_VARARGS, "Iotrace Initialize!"},
	  {"nextrequest", wrap_nextrequest, METH_VARARGS, "Get a request!"},
	  {"finalize", wrap_finalize, METH_VARARGS, "Stop Iotrace!"},
	    {NULL, NULL}
};

void initIotrace() 
{
	  PyObject* m;
	    m = Py_InitModule("Iotrace", IotraceMethods);
}


