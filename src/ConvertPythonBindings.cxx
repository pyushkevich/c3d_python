/*=========================================================================

  Program:   C3D: Command-line companion tool to ITK-SNAP
  Module:    Convert3DMain.cxx
  Language:  C++
  Website:   itksnap.org/c3d
  Copyright (c) 2024 Paul A. Yushkevich

  This file is part of C3D, a command-line companion tool to ITK-SNAP

  C3D is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

=========================================================================*/
#include <ConvertAPI.h>
#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include "pybind11/numpy.h"
#include "pybind11/iostream.h"
#include <iostream>
#include <itkImage.h>
#include <itkImportImageFilter.h>
#include <itkMetaDataObject.h>
namespace py=pybind11;

using Convert3D = ConvertAPI<double, 3>;
using namespace std;

/**
 * This class handles importing data and metadata from SimpleITK images
 */
template <typename TPixel, unsigned int VDim>
class ImageImport
{
public:
  using ImageType = itk::Image<TPixel, VDim>;
  using ImportFilterType = itk::ImportImageFilter<TPixel, VDim>;
  using RegionType = typename ImageType::RegionType;
  using SpacingType = typename ImageType::SpacingType;
  using PointType = typename ImageType::PointType;

  using ImportArray = py::array_t<double, py::array::c_style | py::array::forcecast>;

  ImageImport(py::object sitk_image)
  {
    // Confirm that the image is of correct type
    py::object sitk = py::module_::import("SimpleITK");
    if(!py::isinstance(sitk_image, sitk.attr("Image")))
      throw std::runtime_error("Input not a SimpleITK image!");

    // Check that the number of components is correct
    int ncomp = sitk_image.attr("GetNumberOfComponentsPerPixel")().cast<int>();
    if(ncomp != 1)
      throw std::runtime_error("Vector images are not supported!");

    // Extract the image array from the image
    py::object arr = sitk.attr("GetArrayFromImage")(sitk_image);
    ImportArray arr_c = arr.cast<ImportArray>();

    // Check that the image dimensions are correct
    py::buffer_info arr_info = arr_c.request();
    if(arr_info.ndim != VDim)
      throw std::runtime_error("Incompatible array dimensions!");

    // Get image spacing, origin, direction
    std::array<double, VDim> spacing = sitk_image.attr("GetSpacing")().cast< std::array<double, VDim> >();
    std::array<double, VDim> origin = sitk_image.attr("GetOrigin")().cast< std::array<double, VDim> >();
    std::array<double, VDim*VDim> dir = sitk_image.attr("GetDirection")().cast< std::array<double, VDim*VDim> >();

    typename ImportFilterType::Pointer import = ImportFilterType::New();
    typename ImageType::RegionType itk_region;
    typename ImageType::SpacingType itk_spacing;
    typename ImageType::PointType itk_origin;
    typename ImageType::DirectionType itk_dir;
    int q = 0;
    for(unsigned int i = 0; i < arr_info.ndim; i++)
    {
      itk_region.SetSize(i, arr_info.shape[(VDim-1) - i]); // Shape is reversed between ITK and Numpy
      itk_spacing[i] = spacing[i];
      itk_origin[i] = origin[i];
      for(unsigned int j = 0; j < VDim; j++)
        itk_dir[i][j] = dir[q++];
    }

    import->SetRegion(itk_region);
    import->SetOrigin(itk_origin);
    import->SetSpacing(itk_spacing);
    import->SetDirection(itk_dir);

    // We have to make a copy of the buffer, otherwise the memory may get deallocated
    // TODO: in the future, maybe avoid data duplication?
    TPixel *ptr_copy = new TPixel[arr_info.size];
    memcpy(ptr_copy, arr_c.data(), sizeof(TPixel) * arr_info.size);
    import->SetImportPointer(static_cast<TPixel *>(ptr_copy), arr_info.size, true);
    import->Update();
    this->image = import->GetOutput();

    for (const auto &key : sitk_image.attr("GetMetaDataKeys")()) 
    {
      const auto &value = sitk_image.attr("GetMetaData")(key);
      itk::EncapsulateMetaData<std::string>(
        this->image->GetMetaDataDictionary(), 
        std::string(py::str(key)).c_str(), 
        std::string(py::str(value)).c_str());
    }
  }

  std::string GetInfoString() {
    std::ostringstream oss;
    image->Print(oss);
    return oss.str();
  }

  ImageType* GetImage() const { return image; }

private:
  typename ImageType::Pointer image;
};


template <typename TPixel, unsigned int VDim>
class ImageExport
{
public:
  using ImageType = itk::Image<TPixel, VDim>;
  using ImportFilterType = itk::ImportImageFilter<TPixel, VDim>;
  using RegionType = typename ImageType::RegionType;
  using SpacingType = typename ImageType::SpacingType;
  using PointType = typename ImageType::PointType;

  ImageExport(ImageType* image)
  {
    // SimpleITK 
    py::object sitk = py::module_::import("SimpleITK");

    // Create a numpy array from the image buffer
    std::vector<py::ssize_t> shape(VDim);
    for(unsigned int i = 0; i < 3; i++)
      shape[i] = image->GetBufferedRegion().GetSize((VDim-1) - i);
    py::buffer_info bi(
      image->GetBufferPointer(), sizeof(TPixel),
      py::format_descriptor<TPixel>::format(),
      shape.size(), shape, 
      py::detail::c_strides(shape, sizeof(TPixel)));
    py::array arr(bi);

    // Generate a simple ITK image from this
    this->sitk_image = sitk.attr("GetImageFromArray")(arr, false);

    // Update the spacing, etc
    std::array<double, VDim> spacing, origin;
    std::array<double, VDim*VDim> dir;
    for(unsigned int i = 0, q = 0; i < VDim; i++)
    {
      spacing[i] = image->GetSpacing()[i];
      origin[i] = image->GetOrigin()[i];
      for(unsigned int j = 0; j < VDim; j++, q++)
        dir[q] = image->GetDirection()[i][j];
    }
    this->sitk_image.attr("SetSpacing")(spacing);
    this->sitk_image.attr("SetOrigin")(origin);
    this->sitk_image.attr("SetDirection")(dir);
  }

  py::object sitk_image;
};



template <typename TPixel, unsigned int VDim>
void instantiate_cnd(py::handle m, const char *name)
{
  using Convert = ConvertAPI<TPixel, VDim>;
  using Import = ImageImport<TPixel, VDim>;
  using Export = ImageExport<TPixel, VDim>;

  static std::map< std::pair<Convert *, std::string>, py::object> obj_map;

  py::class_<Convert>(m, name, "Python API for the PICSL c2d/c3d/c4d tool")

    // Constructor that optionally takes the output and error streams
    .def(py::init([](py::object sout = py::none(), py::object serr = py::none()) {
      auto *c = new Convert();
      obj_map[std::make_pair(c, "sout")] = sout; sout.inc_ref();
      obj_map[std::make_pair(c, "serr")] = serr; serr.inc_ref();
      return c;
    }), "Construct a new instance of ConvertND API",
         py::arg("out") = py::none(),
         py::arg("err") = py::none())
    .def("execute", [](Convert &c, const string &cmd, py::object sout, py::object serr) {
      sout = sout.is(py::none()) ? obj_map[std::make_pair(&c, "sout")] : sout;
      sout = sout.is(py::none()) ? py::module_::import("sys").attr("stdout") : sout;
      serr = serr.is(py::none()) ? obj_map[std::make_pair(&c, "serr")] : serr;
      serr = serr.is(py::none()) ? py::module_::import("sys").attr("stderr") : serr;
      py::scoped_ostream_redirect r_out(std::cout, sout);
      py::scoped_ostream_redirect r_err(std::cerr, serr);
      c.ExecuteNoFormatting(cmd.c_str());
    }, "Execute one or more commands using the c3d command line interface",
         py::arg("command"),
         py::arg("out") = py::none(),
         py::arg("err") = py::none())
    .def("add_image", [](Convert &c, const string &var, py::object image) {
      c.AddImage(var.c_str(), Import(image).GetImage());
    }, "Add a SimpleITK image as a named variable available to c3d",
         py::arg("name"), py::arg("image"))
    .def("get_image", [](Convert &c, const string &var) {
      auto image_ptr = c.GetImage(var.c_str());
      Export exp(image_ptr);
      return exp.sitk_image;
    }, "Get a SimpleITK image for a named variable in c3d",
         py::arg("name"))
    .def("push", [](Convert &c, py::object image) {
      c.PushImage(Import(image).GetImage());
    }, "Push a SimpleITK image on the c3d stack",
         py::arg("image"))
    .def("pop", [](Convert &c) {
      auto image_ptr = c.PopImage();
      Export exp(image_ptr);
      return exp.sitk_image;
    }, "Pop from the c3d stack and return as SimpleITK image")
    .def("peek", [](Convert &c, int pos) {
      auto image_ptr = c.PeekImage(pos);
      Export exp(image_ptr);
      return exp.sitk_image;
    }, "Peek at the c3d stack and return as SimpleITK image",
         py::arg("pos"))
    ;
}

PYBIND11_MODULE(picsl_c3d, m) {
  instantiate_cnd<double, 2>(m, "Convert2D");
  instantiate_cnd<double, 3>(m, "Convert3D");
  instantiate_cnd<double, 4>(m, "Convert4D");
};
