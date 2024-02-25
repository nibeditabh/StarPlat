#include"push_relabel.dsl.h"
int src, snk ;

void push__(Graph& g, int u, int v, NodeProperty<int>& excess, 
  EdgeProperty<int>& residual_capacity, boost::mpi::communicator world )
{
  Edge forward_edge = g.get_edge(u, v);
  Edge backward_edge = g.get_edge(v, u);
  int d = std::min(excess.getValue(u),residual_capacity.getValue(forward_edge));
//  printf ("Doing push fron vertex %d to vertex %d with excess of %d and process number %d\n", u, v, excess.getValue(u), world.rank()) ;
  int temp = 0 ;
  temp = (excess.getValue(u) - d);
//  if(world.rank() == g.get_node_owner(u))
//  {
    excess.setValue(u,temp);
//  }
  temp = (excess.getValue(v) + d);
//  if(world.rank() == g.get_node_owner(v))
//  {
    excess.setValue(v,temp);
//  }
  temp = residual_capacity.getValue(forward_edge) - d;
  //if(world.rank() == g.get_edge_owner(forward_edge))
  //{
    residual_capacity.setValue(forward_edge,temp);
  //}
  temp = residual_capacity.getValue(backward_edge) + d;
  //if(world.rank() == g.get_edge_owner(backward_edge))
  //{
    residual_capacity.setValue(backward_edge,temp);
  //}
  if (excess.getValue(v) > 0 && v != src && v != snk )
  {
 //   printf ("pushing node %d into queue of process %d\n", v, world.rank()) ;
    g.frontier_push(v, world);

  }
}

void relabel(Graph& g, int u, EdgeProperty<int>& residue, NodeProperty<int>& label
  , boost::mpi::communicator world )
{

  printf ("Doing relabel of vertex %d with labelof %d by process %d\n", u, label.getValue(u), world.rank() ) ;
  int new_label = INT_MAX;
  for (int v : g.getNeighbors(u)) 
  {
    Edge residual_capacity = g.get_edge(u, v);
    if (residue.getValue(residual_capacity) > 0 )
    {
      if (new_label > label.getValue(v) )
      {
        new_label = label.getValue(v) ;
      }
    }
  }

  if (new_label <= g.num_nodes()+1)
  {
    //if(world.rank() == g.get_node_owner(u))
    //{
      label.setValue(u,new_label + 1);
    //}
  }

}
void discharge(Graph& g, int u, NodeProperty<int>& label, NodeProperty<int>& excess, 
  NodeProperty<int>& curr_edge, EdgeProperty<int>& residue, boost::mpi::communicator world )
{
  while (excess.getValue(u) > 0 ){
  //  printf ("discharging vertex %d with excess = %d at height = %d\n by process id = %d\n", u, excess.getValue(u), label.getValue(u), world.rank() ) ;
    for (int v : g.getNeighbors(u)) 
    {
      Edge current_edge = g.get_edge(u, v);
      if (excess.getValue(u) > 0 && residue.getValue(current_edge) > 0 && label.getValue(u) == label.getValue(v) + 1 )
      {
        push__(g,u,v,excess,residue, world);

      }
    }

    if (excess.getValue(u) > 0 )
    {
      int prevValue = label.getValue(u) ;
      relabel(g,u,residue,label, world);
      if (label.getValue(u) == prevValue) break ;
    }
  }

}
void do_max_flow(Graph& g, int source, int sink, NodeProperty<int>& label, 
  NodeProperty<int>& excess, NodeProperty<int>& curr_edge, EdgeProperty<int>& residue, boost::mpi::communicator world )
{
  src=source, snk=sink ;
  residue = g.weights;
  label.attachToGraph(&g, (int)0);
  excess.attachToGraph(&g, (int)0);
  curr_edge.attachToGraph(&g, (int)0);
  int temp = 0;
  int res = 0;
  for (int v : g.getNeighbors(source)) 
  {
    Edge e = g.get_edge(source, v);
    temp = residue.getValue(e);
    res = res + temp;
    //if (world.rank() == g.get_node_owner(v)) 
    //{
      excess.setValue (v, temp) ;
    //}
    if (world.rank() == g.get_node_owner(v)) {
      g.frontier_push (v, world) ;
 //     printf ("adding %d to queue at process %d\n", v, world.rank()) ;
    }
  }
  world.barrier () ;

//  if(world.rank() == g.get_node_owner(source))
//  {
    label.setValue(source,g.num_nodes( ));
//  }
  while (!g.frontier_empty(world) ){
    int u = g.frontier_pop_local(world);
    if ( u != -1)
    discharge(g,u,label,excess,curr_edge,residue, world);
    world.barrier() ;
  }
  world.barrier () ;
  printf ("status of frontier size = %d\n", g.frontier_empty (world)) ;
  printf ("size of frontier = %d\n", g.frontier_size ()) ;
  printf ("excess at sink = %d\n", excess.getValue (sink)) ;
}
