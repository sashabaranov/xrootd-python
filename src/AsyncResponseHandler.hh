//------------------------------------------------------------------------------
// Copyright (c) 2012-2013 by European Organization for Nuclear Research (CERN)
// Author: Justin Salmon <jsalmon@cern.ch>
//------------------------------------------------------------------------------
// XRootD is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// XRootD is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with XRootD.  If not, see <http://www.gnu.org/licenses/>.
//------------------------------------------------------------------------------

#ifndef ASYNCRESPONSEHANDLER_HH_
#define ASYNCRESPONSEHANDLER_HH_

#include <Python.h>

#include "XrdCl/XrdClXRootDResponses.hh"

#include "XrdClBindUtils.hh"
#include "HostInfoType.hh"

namespace XrdClBind
{
  //----------------------------------------------------------------------------
  //! Generic asynchronous response handler
  //----------------------------------------------------------------------------
  template<class Type = int>
  class AsyncResponseHandler: public XrdCl::ResponseHandler
  {
    public:
      //------------------------------------------------------------------------
      //! Constructor
      //------------------------------------------------------------------------
      AsyncResponseHandler( PyTypeObject *bindType, PyObject *callback ) :
          bindType( bindType ),
          callback( callback ) {}

      //------------------------------------------------------------------------
      //! Handle the asynchronous response call
      //------------------------------------------------------------------------
      void HandleResponseWithHosts( XrdCl::XRootDStatus *status,
                                    XrdCl::AnyObject *response,
                                    XrdCl::HostList *hostList )
      {
        //----------------------------------------------------------------------
        // Ensure we hold the Global Interpreter Lock
        //----------------------------------------------------------------------
        PyGILState_STATE state = PyGILState_Ensure();

        //----------------------------------------------------------------------
        // Convert the XRootDStatus object
        //----------------------------------------------------------------------
        PyObject *statusDict = XRootDStatusDict(status);
        if ( !statusDict || PyErr_Occurred() ) {
          PyErr_Print();
          PyGILState_Release( state );
          return;
        }

        //----------------------------------------------------------------------
        // Convert the response object, if any
        //----------------------------------------------------------------------
        PyObject *responseBind = NULL;
        if ( response != NULL) {
          responseBind = ParseResponse( response );
          if ( responseBind == NULL || PyErr_Occurred() ) {
            PyErr_Print();
            PyGILState_Release( state );
            return;
          }
        }

        //----------------------------------------------------------------------
        // Convert the host list
        //----------------------------------------------------------------------
        PyObject *hostListBind = PyList_New( 0 );

        for ( unsigned int i = 0; i < hostList->size(); ++i ) {

          PyObject *hostInfoBind = ConvertType<XrdCl::HostInfo>(
                                   &hostList->at( i ), &HostInfoType );
          if ( !hostInfoBind || PyErr_Occurred() ) {
            PyErr_Print();
            PyGILState_Release( state );
            return;
          }

          Py_INCREF( hostInfoBind );
          if ( PyList_Append( hostListBind, hostInfoBind ) != 0 ) {
            PyErr_Print();
            PyGILState_Release( state );
            return;
          }
        }

        //----------------------------------------------------------------------
        // Build the callback arguments
        //----------------------------------------------------------------------
        PyObject *args = (responseBind != NULL) ?
            Py_BuildValue( "(OOO)", statusDict, responseBind, hostListBind ) :
            Py_BuildValue( "(OO)",  statusDict, hostListBind );
        if ( !args || PyErr_Occurred() ) {
          PyErr_Print();
          PyGILState_Release( state );
          return;
        }
        //PyObject_Print(responseBind, stdout, 0); printf("\n");
        //PyObject_Print(args, stdout, 0); printf("\n");
        //----------------------------------------------------------------------
        // Invoke the Python callback
        //----------------------------------------------------------------------
        PyObject *callbackResult = PyObject_CallObject( this->callback, args );
        Py_DECREF( args );
        if ( PyErr_Occurred() ) {
          PyErr_Print();
          PyGILState_Release( state );
          return;
        }

        //----------------------------------------------------------------------
        // Clean up
        //----------------------------------------------------------------------
        Py_XDECREF( statusDict );
        Py_XDECREF( responseBind );
        Py_XDECREF( hostListBind );
        Py_XDECREF( callbackResult );
        Py_DECREF( this->callback );

        PyGILState_Release( state );

        delete response;
        delete hostList;
        // Commit suicide...
        delete this;
      }

      //------------------------------------------------------------------------
      //! Parse out and convert the AnyObject response to a mapping type
      //------------------------------------------------------------------------
      PyObject* ParseResponse( XrdCl::AnyObject *response )
      {
        PyObject *responseBind = 0;
        Type     *type = 0;
        response->Get( type );
        responseBind = ConvertType<Type>(type, this->bindType);
        return (!responseBind || PyErr_Occurred()) ? NULL : responseBind;
      }

    private:

      PyTypeObject *bindType;
      PyObject *callback;
  };
}

#endif /* ASYNCRESPONSEHANDLER_HH_ */