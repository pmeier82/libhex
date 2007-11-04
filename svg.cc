#include "hexsvg.h"

#include <cassert>


namespace hex {
namespace svg {


std::string
to_string(const Style& st)
{
  std::string result ="";
  for(Style::const_iterator it=st.begin(); it!=st.end(); ++it)
  {
    if(!result.empty())
        result += ';';
    result += it->first + ":" + it->second;
  }
  return result;
}


/** Helper function. */
template<class InputIterator>
std::ostream&
output_path_data(std::ostream& os, InputIterator first, InputIterator last)
{
  assert(first!=last);
  --last;
  for(InputIterator p =first; p!=last; ++p)
  {
    if(p==first)
        os<<"M "<<(*p);
    else
        os<<" L "<<(*p);
  }
  os<<" Z";
  return os;
}


std::string
Identity::attributes(void) const
{
  std::string result ="";
  if(!this->id.empty())
      result += std::string(" id=\"") + this->id + "\"";
  if(!this->style.empty())
      result += std::string(" style=\"") + to_string(this->style) + "\"";
  if(!this->className.empty())
      result += std::string(" class=\"") + this->className + "\"";
  return result;
}


//
// Poly

Poly::Poly(const std::list<Point>& pp, bool closed, const Identity* identity)
  :_first(pp.begin()), _last(pp.end()), _closed(closed), _identity(identity)
{
  assert(_first!=_last);
  if(closed)
      --_last;
}


std::ostream&
Poly::output(std::ostream& os) const
{
  if(_closed)
      os<<"<polygon";
  else
      os<<"<polyline";
  if(_identity)
      os<<_identity->attributes();
  os<<" points=\"";
  for(std::list<Point>::const_iterator pos=_first; pos!=_last; ++pos)
  {
    if(pos!=_first)
       os<<" ";
    os<<pos->x<<","<<pos->y;
  }
  os<<"\"/>\n";
  return os;
}


//
// SimpleArea

std::ostream&
SimpleArea::output(std::ostream& os, const Area& a) const
{
  Polygon p(a.boundary().stroke(_bias),&a);
  return p.output(os);
}


//
// ComplexArea

std::ostream&
ComplexArea::output(std::ostream& os, const Area& a) const
{
  using namespace std;
  os<<"<path fill-rule=\"nonzero\""<<a.attributes()<<" d=\"";
  const std::list<Point> apoints =a.boundary().stroke(_bias);
  output_path_data(os,apoints.begin(),apoints.end());
  std::list<Area> voids =a.enclosed_areas();
  for(list<Area>::const_iterator v=voids.begin(); v!=voids.end(); ++v)
  {
    os<<" ";
    const std::list<Point> vpoints =v->boundary().stroke(-_bias);
    output_path_data(os,vpoints.rbegin(),vpoints.rend());
  }
  os<<"\"/>\n";
  return os;
}


//
// Skeleton

std::ostream&
Skeleton::output(std::ostream& os, const Area& a) const
{
  os<<"<path"<<a.attributes()<<" d=\"";
  const std::list<Boundary> bb =a.skeleton(this->_include_boundary);
  for(std::list<Boundary>::const_iterator b=bb.begin(); b!=bb.end(); ++b)
  {
    const std::list<Edge*> edges =b->edges();
    assert(!edges.empty());
    os<<"M "<<edges.front()->start_point();
    for(std::list<Edge*>::const_iterator e=edges.begin(); e!=edges.end(); ++e)
        os<<"L "<<(**e).end_point();
  }
  os<<"\"/>\n";
  return os;
}


//
// BoundaryLine

std::ostream&
BoundaryLine::output(std::ostream& os, const Boundary& b) const
{
  os<<Poly(b.stroke(_bias),b.is_closed(),&b);
  return os;
}


//
// PathLine

std::ostream&
PathLine::output(std::ostream& os, const Path& p) const
{
  const std::list<Hex*>& hexes =p.hexes();
  assert(!hexes.empty());
  if(hexes.size()>1) // Nothing to draw if there is only one hex in the path.
  {
    std::list<Point> points;
    for(std::list<Hex*>::const_iterator h =hexes.begin(); h!=hexes.end(); ++h)
        points.push_back( (**h).centre() );
    bool is_closed =( hexes.front()==hexes.back() );
    os<<Poly(points,is_closed,&p);
  }
  return os;
}


//
// Document

void
Document::header(std::ostream& os) const
{
  Distance width  =this->_grid.width();
  Distance height =this->_grid.height();
  Distance hmargin =0.05;
  Distance vmargin =0.05;
  os<<
    "<?xml version=\"1.0\" standalone=\"no\"?>\n"
    "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" "
      "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n"
    "<svg width=\"100%\" height=\"100%\" viewBox=\""
    "0 0 "<<(width+hmargin*2.0)<<" "<<(height+vmargin*2.0)<<
    "\" version=\"1.1\" "
      "xmlns=\"http://www.w3.org/2000/svg\">\n"

    "<defs>\n"
    "<marker id=\"Triangle\""
    " viewBox=\"0 0 10 10\" refX=\"0\" refY=\"5\" "
    " markerUnits=\"strokeWidth\""
    " markerWidth=\"4\" markerHeight=\"3\""
    " orient=\"auto\">\n"
    "<path d=\"M 0 0 L 10 5 L 0 10 z\" />\n"
    "</marker>\n"
    "</defs>\n"

    "<g transform=\"translate("<<hmargin<<" "
      <<(height+vmargin)<<") scale(1 -1)\">\n"
  ;
}


void
Document::footer(std::ostream& os) const
{
  os<<"</g></svg>"<<std::endl;
}


std::ostream&
Document::output(std::ostream& os) const
{
  this->header(os);
  for(std::list<Element*>::const_iterator e =this->elements.begin();
                                          e!=this->elements.end();
                                        ++e)
  {
    (**e).output(os);
  }
  this->footer(os);
  return os;
}


} // end namespace svg
} // end namespace hex
