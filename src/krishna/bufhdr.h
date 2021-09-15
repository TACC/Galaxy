struct bufhdr
{
  enum btype
  {
    None, Particles, Triangles, PathLines
  } type;
  
  int          origin;
  bool         has_data;
  int          npoints;
  int          nconnectivity;
};


