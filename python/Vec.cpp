#include "Module.h"

#include <cstddef>
#include <cstring>

namespace
{
#ifdef BT_USE_DOUBLE_PRECISION
constexpr auto ELEMENT_TYPE = T_DOUBLE;
#else
constexpr auto ELEMENT_TYPE = T_FLOAT;
#endif
}

namespace RocketSim::Python
{
PyTypeObject *Vec::Type = nullptr;

PyMemberDef Vec::Members[] = {
    {.name      = "x",
        .type   = ELEMENT_TYPE,
        .offset = offsetof (Vec, vec) + offsetof (btVector3, m_floats[0]),
        .flags  = 0,
        .doc    = "x"},
    {.name      = "y",
        .type   = ELEMENT_TYPE,
        .offset = offsetof (Vec, vec) + offsetof (btVector3, m_floats[1]),
        .flags  = 0,
        .doc    = "y"},
    {.name      = "z",
        .type   = ELEMENT_TYPE,
        .offset = offsetof (Vec, vec) + offsetof (btVector3, m_floats[2]),
        .flags  = 0,
        .doc    = "z"},
    {.name = nullptr, .type = 0, .offset = 0, .flags = 0, .doc = nullptr},
};

PyMethodDef Vec::Methods[] = {
    {.ml_name = "round", .ml_meth = (PyCFunction)&Vec::Round, .ml_flags = METH_VARARGS, .ml_doc = nullptr},
    {.ml_name = "as_tuple", .ml_meth = (PyCFunction)&Vec::AsTuple, .ml_flags = METH_NOARGS, .ml_doc = nullptr},
    {.ml_name = "__format__", .ml_meth = (PyCFunction)&Vec::Format, .ml_flags = METH_VARARGS, .ml_doc = nullptr},
    {.ml_name = nullptr, .ml_meth = nullptr, .ml_flags = 0, .ml_doc = nullptr},
};

PyType_Slot Vec::Slots[] = {
    {Py_tp_new, (void *)&Vec::New},
    {Py_tp_init, (void *)&Vec::Init},
    {Py_tp_dealloc, (void *)&Vec::Dealloc},
    {Py_tp_repr, (void *)&Vec::Repr},
    {Py_tp_members, &Vec::Members},
    {Py_tp_methods, &Vec::Methods},
    {0, nullptr},
};

PyType_Spec Vec::Spec = {
    .name      = "RocketSim.Vec",
    .basicsize = sizeof (Vec),
    .itemsize  = 0,
    .flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HEAPTYPE,
    .slots     = Vec::Slots,
};

PyObject *Vec::New (PyTypeObject *subtype_, PyObject *args_, PyObject *kwds_) noexcept
{
	auto const tp_alloc = (allocfunc)PyType_GetSlot (subtype_, Py_tp_alloc);

	auto self = PyRef<Vec>::stealObject (tp_alloc (subtype_, 0));
	if (!self)
		return nullptr;

	new (&self->vec) btVector3 (0.0f, 0.0f, 0.0f);

	return self.giftObject ();
}

int Vec::Init (Vec *self_, PyObject *args_, PyObject *kwds_) noexcept
{
	auto vec = btVector3 (0.0f, 0.0f, 0.0f);
	if (!PyArg_ParseTuple (args_, "|fff", &vec.m_floats[0], &vec.m_floats[1], &vec.m_floats[2]))
		return -1;

	self_->vec = vec;

	return 0;
}

void Vec::Dealloc (Vec *self_) noexcept
{
	self_->vec.~btVector3 ();

	auto const tp_free = (freefunc)PyType_GetSlot (Type, Py_tp_free);
	tp_free (self_);
}

PyObject *Vec::Repr (Vec *self_) noexcept
{
	auto const tp_repr = (reprfunc)PyType_GetSlot (&PyTuple_Type, Py_tp_repr);
	if (!tp_repr)
		return nullptr;

	auto tuple = PyObjectRef::steal (Py_BuildValue ("fff", self_->vec.getX (), self_->vec.getY (), self_->vec.getZ ()));
	if (!tuple)
		return nullptr;

	return tp_repr (tuple.borrow ());
}

PyObject *Vec::Format (Vec *self_, PyObject *args_) noexcept
{
	auto tp_methods = (PyMethodDef *)PyType_GetSlot (&PyFloat_Type, Py_tp_methods);
	if (!tp_methods)
		return nullptr;

	PyCFunction format = nullptr;
	while (tp_methods->ml_name)
	{
		if (std::strcmp (tp_methods->ml_name, "__format__") == 0)
		{
			format = tp_methods->ml_meth;
			break;
		}

		++tp_methods;
	}

	if (!format)
		return nullptr;

	PyObject *spec; // borrowed reference
	if (!PyArg_ParseTuple (args_, "O!", &PyUnicode_Type, &spec))
		return nullptr;

	auto const x = PyObjectRef::steal (PyFloat_FromDouble (self_->vec.getX ()));
	if (!x)
		return nullptr;

	auto const xString = PyObjectRef::steal (format (x.borrow (), spec));
	if (!xString)
		return nullptr;

	auto const y = PyObjectRef::steal (PyFloat_FromDouble (self_->vec.getY ()));
	if (!y)
		return nullptr;

	auto const yString = PyObjectRef::steal (format (y.borrow (), spec));
	if (!yString)
		return nullptr;

	auto const z = PyObjectRef::steal (PyFloat_FromDouble (self_->vec.getZ ()));
	if (!z)
		return nullptr;

	auto const zString = PyObjectRef::steal (format (z.borrow (), spec));
	if (!zString)
		return nullptr;

	return PyUnicode_FromFormat ("(%S, %S, %S)", xString.borrow (), yString.borrow (), zString.borrow ());
}

PyObject *Vec::AsTuple (Vec *self_) noexcept
{
	return Py_BuildValue ("fff", self_->vec.getX (), self_->vec.getY (), self_->vec.getZ ());
}

PyObject *Vec::Round (Vec *self_, PyObject *args_) noexcept
{
	float precision;
	if (!PyArg_ParseTuple (args_, "f", &precision))
		return nullptr;

	auto result = PyRef<Vec>::stealObject (Vec::New (Vec::Type, nullptr, nullptr));

	result->vec = Math::RoundVec (self_->vec, precision);

	return result.giftObject ();
}
}