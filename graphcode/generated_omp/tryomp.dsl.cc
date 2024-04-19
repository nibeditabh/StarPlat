#include"tryomp.dsl.h"

auto find(int u , int* parent)
{
  int par = parent[u];
  while (u != par ){
    u = par;
    par = parent[u];
  }
  return par;

}
void merge(int u , int v , int* parent , int* rnk
)
{
  u = find(u,parent);
  v = find(v,parent);
  if (rnk[u] > rnk[v] )
    {
    parent[v] = u;
  }
  else
  {
    parent[u] = v;
  }
  if (rnk[u] == rnk[v] )
    {
    int temp = rnk[v];
    rnk[v] = temp + 1;
  }

}
auto qi_seq(std::vector<int> degree , std::vector<int> edges , int n , int* parent , 
  int* rnk)
{
  std::vector<std::vector<int>> records;
  std::vector<int> par;
  int vertex = 0;
  int index = 0;
  while (vertex < n ){
    std::vector<int> temp;
    records.push_back(temp);

    par.push_back(-1);

    vertex++;
  }
  vertex = 0;
  while (vertex < n ){
    int deg = degree[vertex];
    while (deg > 0 ){
      int set_u = find(vertex,parent);
      int set_v = find(edges[index],parent);
      if (set_u != set_v )
        {
        merge(set_u,set_v,parent,rnk);

        int dst = edges[index];
        par[dst] = vertex;
      }
      else
      {
        int dst = edges[index];
        records[vertex].push_back(dst);

      }
      index++;
      deg--;
    }
    vertex++;
  }
  vertex = 0;
  while (vertex < n ){
    records[vertex].push_back(par[vertex]);

    vertex++;
  }
  return records;

}
auto quicksi(std::vector<int> degree , std::vector<std::vector<int>> records , graph& g , int d , 
  int* H , int* F)
{
  if (d >= records.size() )
    {
    return true;
  }
  std::vector<int> temp;
  temp = records[d];
  bool res = false;
  int par = temp.back();
  #pragma omp parallel for
  for (int v = 0; v < g.num_nodes(); v ++) 
  {
    if (!res && ((d == 0 && F[v] == 0) || (d > 0 && F[v] == 0 && (par == -1 || (par != -1 && g.check_if_nbr(H[par],v))))) )
      {
      if (g.getOutDegree(v) >= degree[d] )
        {
        bool flag = true;
        int index = 0;
        while (index < temp.size() - 1 ){
          int val = temp[index];
          if (flag )
            {
            if (H[val] != -1 && !g.check_if_nbr(v,H[val]) )
              {
              flag = false;
            }
          }
          index++;
        }
        if (flag )
          {
          H[d] = v;
          F[v] = 1;
          if (quicksi(degree,records,g,d + 1,H,F) )
            {
            res = true;
          }
          F[v] = 0;
        }
      }
    }
  }
  return res;

}
