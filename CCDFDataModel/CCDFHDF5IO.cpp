/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include "CCDFHDF5IO.h"
const char *CDFHDF5Reader::className = "CDFHDF5Reader";
int CDFHDF5Reader::convertNWCSAFtoCF() {
  if (cdfObject->getAttributeNE("PROJECTION") == NULL) {
    // Silently SKIP, this is not NWC SAF data.
    return 0;
  };

  bool dimsDone = false;
  for (size_t j = 0; j < cdfObject->variables.size(); j++) {
    cdfObject->variables[j]->setAttributeText("ADAGUC_SKIP", "true");
    CT::string projectionString = "";
    try {
      if (cdfObject->variables[j]->getAttribute("CLASS")->toString().toLowerCase().equals("image")) {
        cdfObject->variables[j]->removeAttribute("ADAGUC_SKIP");
        CDBDebug("Variable %s is an IMAGE", cdfObject->variables[j]->name.c_str());
        if (dimsDone == false) {

          float fXGEO_UP_LEFT[1];
          float fYGEO_UP_LEFT[1];
          float fXGEO_LOW_RIGHT[1];
          float fYGEO_LOW_RIGHT[1];

          CT::string timeString = "";

          CDF::Attribute *NOMINAL_PRODUCT_TIME = cdfObject->getAttributeNE("NOMINAL_PRODUCT_TIME");
          if (NOMINAL_PRODUCT_TIME != NULL) {
            timeString = NOMINAL_PRODUCT_TIME->toString();
          } else {
            CDBError("NOMINAL_PRODUCT_TIME  not found");
            return 1;
          }
          CDF::Attribute *PROJECTION = cdfObject->getAttributeNE("PROJECTION");
          if (PROJECTION != NULL) {
            projectionString = PROJECTION->toString();
          } else {
            CDBError("PROJECTION  not found");
            return 1;
          }
          CDF::Attribute *XGEO_UP_LEFT = cdfObject->getAttributeNE("XGEO_UP_LEFT");
          if (XGEO_UP_LEFT != NULL) {
            XGEO_UP_LEFT->getData(fXGEO_UP_LEFT, 1);
          } else {
            CDBError("XGEO_UP_LEFT not found");
            return 1;
          }
          CDF::Attribute *YGEO_UP_LEFT = cdfObject->getAttributeNE("YGEO_UP_LEFT");
          if (YGEO_UP_LEFT != NULL) {
            YGEO_UP_LEFT->getData(fYGEO_UP_LEFT, 1);
          } else {
            CDBError("YGEO_UP_LEFT not found");
            return 1;
          }
          CDF::Attribute *XGEO_LOW_RIGHT = cdfObject->getAttributeNE("XGEO_LOW_RIGHT");
          if (XGEO_LOW_RIGHT != NULL) {
            XGEO_LOW_RIGHT->getData(fXGEO_LOW_RIGHT, 1);
          } else {
            CDBError("XGEO_LOW_RIGHT not found");
            return 1;
          }
          CDF::Attribute *YGEO_LOW_RIGHT = cdfObject->getAttributeNE("YGEO_LOW_RIGHT");
          if (YGEO_LOW_RIGHT != NULL) {
            YGEO_LOW_RIGHT->getData(fYGEO_LOW_RIGHT, 1);
          } else {
            CDBError("YGEO_LOW_RIGHT not found");
            return 1;
          }
          CDBDebug("NOMINAL_PRODUCT_TIME = %s", timeString.c_str());
          CDBDebug("PROJECTION = %s", projectionString.c_str());
          CDBDebug("XGEO_UP_LEFT = %f", fXGEO_UP_LEFT[0]);
          CDBDebug("YGEO_UP_LEFT = %f", fYGEO_UP_LEFT[0]);
          CDBDebug("XGEO_LOW_RIGHT = %f", fXGEO_LOW_RIGHT[0]);
          CDBDebug("YGEO_LOW_RIGHT = %f", fYGEO_LOW_RIGHT[0]);

          CDF::Dimension *dimX = cdfObject->variables[j]->dimensionlinks[1];
          CDF::Dimension *dimY = cdfObject->variables[j]->dimensionlinks[0];
          CDF::Variable *varX = cdfObject->getVariableNE(dimX->name.c_str());
          CDF::Variable *varY = cdfObject->getVariableNE(dimY->name.c_str());

          if (varX == NULL) {
            CDBError("XDimension not found");
            return 1;
          }
          if (varY == NULL) {
            CDBError("YDimension not found");
            return 1;
          }

          if (CDF::allocateData(varX->currentType, &varX->data, dimX->length)) {
            throw(__LINE__);
          }
          if (CDF::allocateData(varY->currentType, &varY->data, dimY->length)) {
            throw(__LINE__);
          }

          varX->setName("x");
          varY->setName("y");
          dimX->setName("x");
          dimY->setName("y");

#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("Creating virtual dimensions x and y");
#endif

          double cellSizeX = (fXGEO_LOW_RIGHT[0] - fXGEO_UP_LEFT[0]) / float(dimX->length);
          double cellSizeY = (fYGEO_LOW_RIGHT[0] - fYGEO_UP_LEFT[0]) / float(dimY->length);
          double offsetX = fXGEO_UP_LEFT[0];
          double offsetY = fYGEO_UP_LEFT[0];

          CDBDebug("Cellsize X = %f", cellSizeX);
          CDBDebug("Cellsize Y = %f", cellSizeY);

          for (size_t j = 0; j < dimX->length; j = j + 1) {
            double x = offsetX + (double(j)) * cellSizeX + cellSizeX / 2;
            ((double *)varX->data)[j] = x;
          }

          for (size_t j = 0; j < dimY->length; j = j + 1) {
            double y = offsetY + (float(j)) * cellSizeY + cellSizeY / 2;
            ((double *)varY->data)[j] = y;
          }

          CDF::Variable *projection = NULL;
          projection = cdfObject->getVariableNE("projection");
          if (projection == NULL) {
            projection = new CDF::Variable();
            cdfObject->addVariable(projection);
            projection->setName("projection");
            projection->currentType = CDF_CHAR;
            projection->nativeType = CDF_CHAR;
            projection->isDimension = false;
          }
#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("CProj4ToCF");
#endif
          CProj4ToCF proj4ToCF;
          proj4ToCF.convertProjToCF(projection, projectionString.c_str());
#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("/CProj4ToCF");
#endif
          CDF::Attribute *proj4_params = projection->getAttributeNE("proj4_params");
          if (proj4_params == NULL) {
            proj4_params = new CDF::Attribute();
            projection->addAttribute(proj4_params);
            proj4_params->setName("proj4_params");
          }
          proj4_params->setData(CDF_CHAR, projectionString.c_str(), projectionString.length());

          // Set time dimension
          CDF::Variable *time = new CDF::Variable();
          CDF::Dimension *timeDim = new CDF::Dimension();
          cdfObject->addDimension(timeDim);
          cdfObject->addVariable(time);
          time->setName("time");
          timeDim->setName("time");
          timeDim->length = 1;
          time->currentType = CDF_DOUBLE;
          time->nativeType = CDF_DOUBLE;
          time->isDimension = true;
          CDF::Attribute *time_units = new CDF::Attribute();
          time_units->setName("units");
          time_units->setData("minutes since 2000-01-01 00:00:00\0");
          time->addAttribute(time_units);

          CDF::Attribute *standard_name = new CDF::Attribute();
          standard_name->setName("standard_name");
          standard_name->setData("time");
          time->addAttribute(standard_name);

          time->dimensionlinks.push_back(timeDim);

          // Set adaguc time
          CTime *ctime = new CTime();
          if (ctime->init((char *)time_units->data, NULL) != 0) {
            CDBError("Could not initialize CTIME: %s", (char *)time_units->data);
            return 1;
          }
          double offset;
          try {
            offset = ctime->dateToOffset(ctime->freeDateStringToDate(timeString.c_str()));
          } catch (int e) {
            CT::string message = CTime::getErrorMessage(e);
            CDBError("CTime Exception %s", message.c_str());
            delete ctime;
            return 1;
          }

          time->setSize(1);
          if (CDF::allocateData(time->currentType, &time->data, time->getSize())) {
            throw(__LINE__);
          }
          ((double *)time->data)[0] = offset;
          if (status != 0) {
            CDBError("Could not initialize time: %s", timeString.c_str());
            delete ctime;
            return 1;
          }
          delete ctime;

          dimsDone = true;
        }
        CDF::Attribute *grid_mapping = new CDF::Attribute();
        grid_mapping->setName("grid_mapping");
        grid_mapping->setData(CDF_CHAR, (char *)"projection\0", 11);
        cdfObject->variables[j]->addAttribute(grid_mapping);

        cdfObject->variables[j]->dimensionlinks.insert(cdfObject->variables[j]->dimensionlinks.begin(), 1, cdfObject->getDimension("time"));

        // Scale and offset

        CDF::Attribute *SCALING_FACTOR = cdfObject->variables[j]->getAttributeNE("SCALING_FACTOR");
        CDF::Attribute *OFFSET = cdfObject->variables[j]->getAttributeNE("OFFSET");
        if (SCALING_FACTOR != NULL && OFFSET != NULL) {
          float additionFactor[1];
          float multiplicationFactor[1];

          SCALING_FACTOR->getData(multiplicationFactor, 1);
          OFFSET->getData(additionFactor, 1);

          CDF::Attribute *add_offset = new CDF::Attribute();
          add_offset->setName("add_offset");
          add_offset->setData(CDF_FLOAT, additionFactor, 1);
          cdfObject->variables[j]->addAttribute(add_offset);

          CDF::Attribute *scale_factor = new CDF::Attribute();
          scale_factor->setName("scale_factor");
          scale_factor->setData(CDF_FLOAT, multiplicationFactor, 1);
          cdfObject->variables[j]->addAttribute(scale_factor);
        }

        if (cdfObject->variables[j]->getType() == CDF_UBYTE) {
          unsigned char FfillValue = 0;
          CDF::Attribute *fillValue = new CDF::Attribute();
          fillValue->setName("_FillValue");
          fillValue->setData(CDF_UBYTE, &FfillValue, 1);
          cdfObject->variables[j]->addAttribute(fillValue);
        }
      }
    } catch (int e) {
    }
  }
  return 0;
}

int CDFHDF5Reader::convertLSASAFtoCF() {
  if (cdfObject->getAttributeNE("PROJECTION_NAME") == NULL) {
    // Silently SKIP, this is not LSA SAF data.
    return 0;
  };

  bool dimsDone = false;
  for (size_t j = 0; j < cdfObject->variables.size(); j++) {
    cdfObject->variables[j]->setAttributeText("ADAGUC_SKIP", "true");
    CT::string projectionString = "";
    try {
      if (cdfObject->variables[j]->getAttribute("CLASS")->toString().toLowerCase().equals("data")) {
        cdfObject->variables[j]->removeAttribute("ADAGUC_SKIP");
        // CDBDebug("Variable %s is an IMAGE",cdfObject->variables[j]->name.c_str());
        if (dimsDone == false) {

          CT::string timeString = "";

          CDF::Attribute *SENSING_START_TIME = cdfObject->getAttributeNE("SENSING_START_TIME");
          if (SENSING_START_TIME != NULL) {
            timeString = SENSING_START_TIME->toString();
          } else {
            CDBError("SENSING_START_TIME  not found");
            return 1;
          }
          CDF::Attribute *PROJECTION = cdfObject->getAttributeNE("PROJECTION_NAME");
          if (PROJECTION != NULL) {
            projectionString = PROJECTION->toString();
          } else {
            CDBError("PROJECTION  not found");
            return 1;
          }

          float fXGEO_UP_LEFT;
          fXGEO_UP_LEFT = -922500;
          float fYGEO_UP_LEFT;
          fYGEO_UP_LEFT = 5422500;
          float fXGEO_LOW_RIGHT;
          fXGEO_LOW_RIGHT = 4180500;
          float fYGEO_LOW_RIGHT;
          fYGEO_LOW_RIGHT = 3469500;

          //           CDBDebug("SENSING_START_TIME = %s",timeString.c_str());
          //           CDBDebug("PROJECTION = %s",projectionString.c_str());
          //           CDBDebug("XGEO_UP_LEFT = %f",fXGEO_UP_LEFT);
          //           CDBDebug("YGEO_UP_LEFT = %f",fYGEO_UP_LEFT);
          //           CDBDebug("XGEO_LOW_RIGHT = %f",fXGEO_LOW_RIGHT);
          //           CDBDebug("YGEO_LOW_RIGHT = %f",fYGEO_LOW_RIGHT);
          //
          CDF::Dimension *dimX = cdfObject->variables[j]->dimensionlinks[1];
          CDF::Dimension *dimY = cdfObject->variables[j]->dimensionlinks[0];
          CDF::Variable *varX = cdfObject->getVariableNE(dimX->name.c_str());
          CDF::Variable *varY = cdfObject->getVariableNE(dimY->name.c_str());

          if (varX == NULL) {
            CDBError("XDimension not found");
            return 1;
          }
          if (varY == NULL) {
            CDBError("YDimension not found");
            return 1;
          }

          if (CDF::allocateData(varX->currentType, &varX->data, dimX->length)) {
            throw(__LINE__);
          }
          if (CDF::allocateData(varY->currentType, &varY->data, dimY->length)) {
            throw(__LINE__);
          }

          varX->setName("x");
          varY->setName("y");
          dimX->setName("x");
          dimY->setName("y");

#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("Creating virtual dimensions x and y");
#endif

          double cellSizeX = (fXGEO_LOW_RIGHT - fXGEO_UP_LEFT) / float(dimX->length);
          double cellSizeY = (fYGEO_LOW_RIGHT - fYGEO_UP_LEFT) / float(dimY->length);
          double offsetX = fXGEO_UP_LEFT;
          double offsetY = fYGEO_UP_LEFT;

          CDBDebug("Cellsize X = %f", cellSizeX);
          CDBDebug("Cellsize Y = %f", cellSizeY);

          for (size_t j = 0; j < dimX->length; j = j + 1) {
            double x = offsetX + (double(j)) * cellSizeX + cellSizeX / 2;
            ((double *)varX->data)[j] = x;
          }

          for (size_t j = 0; j < dimY->length; j = j + 1) {
            double y = offsetY + (float(j)) * cellSizeY + cellSizeY / 2;
            ((double *)varY->data)[j] = y;
          }

          CDF::Variable *projection = NULL;
          projection = cdfObject->getVariableNE("projection");
          if (projection == NULL) {
            projection = new CDF::Variable();
            cdfObject->addVariable(projection);
            projection->setName("projection");
            projection->currentType = CDF_CHAR;
            projection->nativeType = CDF_CHAR;
            projection->isDimension = false;
          }
#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("CProj4ToCF");
#endif
          CProj4ToCF proj4ToCF;
          proj4ToCF.convertProjToCF(projection, projectionString.c_str());
#ifdef CCDFHDF5IO_DEBUG
          CDBDebug("/CProj4ToCF");
#endif
          CDF::Attribute *proj4_params = projection->getAttributeNE("proj4_params");
          if (proj4_params == NULL) {
            proj4_params = new CDF::Attribute();
            projection->addAttribute(proj4_params);
            proj4_params->setName("proj4_params");
          }
          proj4_params->setData("+proj=geos +a=6378169.0 +b=6356583.8 +lon_0=0.0 +h=35785831.0");

          // Set time dimension

          CDF::Variable *time = new CDF::Variable();
          CDF::Dimension *timeDim = new CDF::Dimension();
          cdfObject->addDimension(timeDim);
          cdfObject->addVariable(time);
          time->setName("time");
          timeDim->setName("time");
          timeDim->length = 1;
          time->currentType = CDF_DOUBLE;
          time->nativeType = CDF_DOUBLE;
          time->isDimension = true;
          CDF::Attribute *time_units = new CDF::Attribute();
          time_units->setName("units");
          time_units->setData("minutes since 2000-01-01 00:00:00\0");
          time->addAttribute(time_units);

          CDF::Attribute *standard_name = new CDF::Attribute();
          standard_name->setName("standard_name");
          standard_name->setData("time");
          time->addAttribute(standard_name);

          time->dimensionlinks.push_back(timeDim);

          // Set adaguc time
          CTime *ctime = new CTime();
          if (ctime->init((char *)time_units->data, NULL) != 0) {
            CDBError("Could not initialize CTIME: %s", (char *)time_units->data);
            return 1;
          }
          double offset;
          try {
            offset = ctime->dateToOffset(ctime->freeDateStringToDate(timeString.c_str()));
          } catch (int e) {
            CT::string message = CTime::getErrorMessage(e);
            CDBError("CTime Exception %s", message.c_str());
            delete ctime;
            return 1;
          }

          time->setSize(1);
          if (CDF::allocateData(time->currentType, &time->data, time->getSize())) {
            throw(__LINE__);
          }
          ((double *)time->data)[0] = offset;
          if (status != 0) {
            CDBError("Could not initialize time: %s", timeString.c_str());
            delete ctime;
            return 1;
          }
          delete ctime;

          dimsDone = true;
        }
        CDF::Attribute *grid_mapping = new CDF::Attribute();
        grid_mapping->setName("grid_mapping");
        grid_mapping->setData(CDF_CHAR, (char *)"projection\0", 11);
        cdfObject->variables[j]->addAttribute(grid_mapping);

        cdfObject->variables[j]->dimensionlinks.insert(cdfObject->variables[j]->dimensionlinks.begin(), 1, cdfObject->getDimension("time"));

        // Scale and offset

        CDF::Attribute *SCALING_FACTOR = cdfObject->variables[j]->getAttributeNE("SCALING_FACTOR");
        CDF::Attribute *OFFSET = cdfObject->variables[j]->getAttributeNE("OFFSET");
        if (SCALING_FACTOR != NULL && OFFSET != NULL) {
          float additionFactor[1];
          float multiplicationFactor[1];

          if (SCALING_FACTOR->size() == 1) {
            SCALING_FACTOR->getData(multiplicationFactor, 1);
            CDBDebug("Setting offset to %f and %f", multiplicationFactor[0]);
            CDF::Attribute *scale_factor = new CDF::Attribute();
            scale_factor->setName("scale_factor");
            scale_factor->setData(CDF_FLOAT, multiplicationFactor, 1);
            cdfObject->variables[j]->addAttribute(scale_factor);
          }

          // FOR LSA, scaling factor should be inversed. This is probably an error in the LSA files!
          if (SCALING_FACTOR->size() == 2) {
            SCALING_FACTOR->getData(multiplicationFactor, 1);
            if (multiplicationFactor[0] != 0) {
              multiplicationFactor[0] = 1 / multiplicationFactor[0];
              CDBDebug("Setting offset to %f and %f", multiplicationFactor[0]);
              CDF::Attribute *scale_factor = new CDF::Attribute();
              scale_factor->setName("scale_factor");
              scale_factor->setData(CDF_FLOAT, multiplicationFactor, 1);
              cdfObject->variables[j]->addAttribute(scale_factor);
            }
          }

          if (OFFSET->size() == 1) {
            OFFSET->getData(additionFactor, 1);
            CDBDebug("Setting offset to %f and %f", additionFactor[0]);
            CDF::Attribute *add_offset = new CDF::Attribute();
            add_offset->setName("add_offset");
            add_offset->setData(CDF_FLOAT, additionFactor, 1);
            cdfObject->variables[j]->addAttribute(add_offset);
          }
        }

        CDF::Attribute *UNITS = cdfObject->variables[j]->getAttributeNE("UNITS");
        if (UNITS != NULL) {
          cdfObject->variables[j]->setAttributeText("units", UNITS->toString().c_str());
        }

        CDF::Attribute *MISSING_VALUE = cdfObject->variables[j]->getAttributeNE("MISSING_VALUE");
        if (MISSING_VALUE != NULL) {
          MISSING_VALUE->setName("_FillValue");
        } else if (cdfObject->variables[j]->getType() == CDF_UBYTE) {
          unsigned char FfillValue = 0;
          CDF::Attribute *fillValue = new CDF::Attribute();
          fillValue->setName("_FillValue");
          fillValue->setData(CDF_UBYTE, &FfillValue, 1);
          cdfObject->variables[j]->addAttribute(fillValue);
        }
      }
    } catch (int e) {
    }
  }
  return 0;
}

int CDFHDF5Reader::readAttributes(std::vector<CDF::Attribute *> &attributes, hid_t HDF5_group) {
  hid_t HDF5_attr_class, HDF5_attribute;
  int dNumAttributes = H5Aget_num_attrs(HDF5_group);
  for (int j = 0; j < dNumAttributes; j++) {
    HDF5_attribute = H5Aopen_idx(HDF5_group, j);
    size_t attNameSize = H5Aget_name(HDF5_attribute, 0, NULL);
    attNameSize++;
    char attName[attNameSize + 1];
    H5Aget_name(HDF5_attribute, attNameSize, attName);
    hid_t HDF5_attr_type = H5Aget_type(HDF5_attribute);
    HDF5_attr_class = H5Tget_class(HDF5_attr_type);
    // Read H5T_INTEGER Attribute
    if (HDF5_attr_class == H5T_INTEGER && 1 == 1) {
      hid_t HDF5_attr_memtype = H5Tget_native_type(HDF5_attr_type, H5T_DIR_ASCEND);
      hsize_t stSize = H5Aget_storage_size(HDF5_attribute) / sizeof(int);
      CDF::Attribute *attr = new CDF::Attribute();
      attributes.push_back(attr);
      attr->setName(attName);

      attr->type = typeConversion(HDF5_attr_memtype);
      attr->length = stSize;
      if (attr->length == 0) attr->length = 1; // TODO
      // printf("%s %d\n",attName,stSize);
      if (CDF::allocateData(attr->type, &attr->data, attr->length + 1) != 0) {
        CDBError("Unable to allocateData for attribute %s", attr->name.c_str());
        throw(__LINE__);
      }

      status = H5Aread(HDF5_attribute, HDF5_attr_memtype, attr->data);

      status = H5Tclose(HDF5_attr_memtype);
    }

    if (HDF5_attr_class == H5T_FLOAT && 1 == 1) {
      hid_t HDF5_attr_memtype = H5Tget_native_type(HDF5_attr_type, H5T_DIR_ASCEND);
      hsize_t stSize = H5Aget_storage_size(HDF5_attribute) / sizeof(float);
      CDF::Attribute *attr = new CDF::Attribute();
      attr->setName(attName);
      attr->type = CDF_FLOAT;
      attr->length = stSize;
      if (CDF::allocateData(attr->type, &attr->data, attr->length + 1)) {
        throw(__LINE__);
      }

      status = H5Aread(HDF5_attribute, H5T_NATIVE_FLOAT, attr->data);
      attributes.push_back(attr);
      status = H5Tclose(HDF5_attr_memtype);
    }
    if (HDF5_attr_class == H5T_STRING) {
      hid_t HDF5_attr_memtype = H5Tget_native_type(HDF5_attr_type, H5T_DIR_ASCEND);

      bool isVariableLength = (H5Tis_variable_str(HDF5_attr_memtype) > 0);
      CDF::Attribute *attr = new CDF::Attribute();
      attr->setName(attName);
      attr->type = CDF_CHAR;
      if (!isVariableLength) {
        hsize_t stSize = H5Aget_storage_size(HDF5_attribute) * sizeof(char);
        attr->length = stSize;
        if (CDF::allocateData(attr->type, &attr->data, attr->length + 1)) {
          throw(__LINE__);
        }
        status = H5Aread(HDF5_attribute, HDF5_attr_type, attr->data);
        size_t stringLength = strlen((char *)attr->data);
        if (attr->length > stringLength) attr->length = stringLength + 1;
        ((char *)attr->data)[attr->length] = '\0';
      } else {
        hid_t space = H5Aget_space(HDF5_attribute);
        hsize_t dims[1] = {1};
        int length = dims[0] * sizeof(char *);
        char **rdata = (char **)malloc(length);
        status = H5Aread(HDF5_attribute, HDF5_attr_type, rdata);
        if (status != -1) {
          attr->length = strlen(rdata[0]);
          if (CDF::allocateData(attr->type, &attr->data, attr->length + 1)) {
            throw(__LINE__);
          }
          memcpy((char *)attr->data, rdata[0], strlen(rdata[0]));
          H5Dvlen_reclaim(HDF5_attr_type, space, H5P_DEFAULT, rdata);
        }
        H5Sclose(space);
      }

      attributes.push_back(attr);
      status = H5Tclose(HDF5_attr_memtype);
    }

    H5Tclose(HDF5_attr_type);
    H5Aclose(HDF5_attribute);
  }
  return 0;
}