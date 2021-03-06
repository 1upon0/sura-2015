/** \file*/
#include <iostream>
#include <cstdio>
#include <vector>
#include <algorithm>
#include <string>
#include <cassert>
#include <cstdint>
#include <queue>
#include <limits>
#include <cfloat>
#include <unordered_map>
#include <map>
#include "common.h"
using namespace std;

#define THRESHOLD (10)
#define MAX_NEIGHBOURS (12)
#define THRESHOLD2 (2000)
int CONTROL_POINTS;
#define FLOAT_MAX (1e9)
#define mp make_pair
void histogramEqualization(Image* img);
/**
 * @brief Distance between two colors
 * @details Used euclidean distance. Can be modified to taxicab(N-4) distance or N-8 distance
 */
float dist(const Color& c1,const Color& c2){
	return sqrt((c1.x-c2.x)*(c1.x-c2.x) + (c1.y-c2.y)*(c1.y-c2.y) + (c1.z-c2.z)*(c1.z-c2.z));
}

float distSq(const Color& c1,const Color& c2){
  return (c1.x-c2.x)*(c1.x-c2.x) + (c1.y-c2.y)*(c1.y-c2.y) + (c1.z-c2.z)*(c1.z-c2.z);
}

// float dist(const Color& c1,const Color& c2){
//   return sqrt(pow(c1.r-c2.r,4) + pow(c1.g-c2.g,4) + pow(c1.b-c2.b,4));
// }

Color aggregateColors(vector<Color> &colors)
{
  float sum;
  if(colors.size()==0)
    return Color(0,0,0);
  Color argMin=colors[0];
  vector< pair<float,Color> > sums;
  for(auto &c1:colors){
    sum=0;
    for(auto &c2:colors){
      sum+=distSq(c1,c2);
    }
    if(sum<0.1)
      sum=0.1;
    sums.push_back({10000.f/sum,c1});
  }
  make_heap(sums.begin(),sums.end());
  size_t maxi=sums.size()*0.1;
  Color totC(0,0,0);
  float totW=0;
  if(maxi<1)
    maxi=1;
  for(int i=0;i<maxi;i++){
    auto sc=sums.front();
    totC = totC + sc.second*sc.first;
    totW += sc.first;
    pop_heap(sums.begin(),sums.end());
    sums.pop_back();
  }
  // totC.print();
  // cout<<totW<<" ";
  return totC/totW;
}

/**
 * @brief Dijkstra shortest path
 * @details Calculates the shortest path from source vertex to all vertices in graph
 *
 * @param beg Source vertex
 * @param n Total number of vertics in graph
 * @param cur cur[i] represent path length to reach vertex i at some point in algorithm. Finally it contains shortest path.
 * @param e Adjanceny of graph. e[i] contains list of all neighbours along with edge cost
 */
void dijkstra(int beg,int n,vector<vector<pair<float,int> > > &e,vector<float> &cur){
	priority_queue< pair<float,int>, vector<pair<float,int> >, greater< pair<float,int> > >  q;
	int node,i;
	float cost;
	q.push(mp(0.0,beg));
	cur[beg]=0;
	while(!q.empty()){
		cost=q.top().first;
		node=q.top().second;
		q.pop();
		if(cur[node]<cost)continue;
		for(i=0;i<e[node].size();i++){
			if(cur[e[node][i].second]>cost+e[node][i].first){
				cur[e[node][i].second]=cost+e[node][i].first;
				q.push(mp(cost+e[node][i].first,e[node][i].second));
			}
		}
	}
}
/**
 * color_map[i] - Represent color with its frequency in the image. This gives us information about histogram of the image.
 */
map<Color,int> color_map;
/**
 * colors - Represent unique color of image. We here do not consider frequency of colors.
 */
vector<Color> colors;
vector<vector<pair<float,int> > > adj;
int main(int argc, char **argv){
	if(argc<2){
		puts("Usage: <executable> <file_name>");
		return 1;
	}

	string name(argv[1]);
	Image* map = new Image();
	map->load(name);
	puts("Dimestion of the image(width,height):");
	cout<<map->w<<" "<<map->h<<endl;


	puts("Getting the control points...");
	int x,y;
	float corrosion_value;
	/**
	 * cv represents control point in color space.
	 * cv[i] - (idx,val) where idx = index/position of color in unique color array, val = corrosion value
	 * We thus get color information corresponding to corrosion value.
	 */
	vector<pair<int,float> > cv(0);
	/**
	 *	cp represents control points in the pixel space.
	 *	cp[i] - (x,y) where x,y are coordinates of control points entered by user
	 */
	vector<pair<int,int> > cp(0);
	while(1){
		puts("Enter x y val (-1 to end):");
		cin>>x;
    if(!cin || x<0)
      break;
		cin>>y>>corrosion_value;
		cv.push_back(mp(0,corrosion_value));
		cp.push_back(mp(x,y));
	}
  CONTROL_POINTS=cp.size();
	puts("Control points done.");


	/**
	 * @brief Histogram processing of the image
	 * @details Extracting unique color from the image and number of pixel corresponding to each color.
	 */
	puts("Processing image. Obtaining color frequency...");
	color_map.clear();
	Color c(0,0,0,0);
	for(int i=0;i<map->w;i++){
		for(int j=0;j<map->h;j++){
			c = map->get(i,j);
			auto it = color_map.find(c);
			if(it==color_map.end()){
				color_map.insert(pair<Color,int>(c,1));
			}
			else{
				it->second++;
			}
		}
	}
	puts("Color frequency done.");

	/**
	 * Obtain unique color neglecting frequency.
	 * Also corresponding to each control point marked by the user, find corrosion value. This gives us rough information of which color represents what degree of corrosion.(Entirely based on user interaction. We are not extracting this information from image.)
   */
	puts("Getting unique color...");
	for(auto _maps : color_map){
		for(int i=0;i<CONTROL_POINTS;i++){
			if(_maps.first==map->get(cp[i].first,cp[i].second)) cv[i].first=colors.size();
		}
		colors.push_back(_maps.first);
	}
	cout<<"Number of colors="<<colors.size()<<endl;
	puts("Unique color done.");


	/**
	 * Generating the graph
	 * For each color c, we calculate distance of all the colors from c. Only those color whose value is less then some threshold, we consider it to be neighbour of c. This way adjanceny list of graph is generated.
	 */
	puts("Generating the graph...");
	int N = colors.size();
	float d;
	adj.resize(N,vector<pair<float,int> >(0));
	for(int i=0;i<N;i++){
		for(int j=i+1;j<N;j++){
			d = dist(colors[i],colors[j]);
			if(d<=THRESHOLD){
				adj[i].push_back(make_pair(d,j));
				adj[j].push_back(make_pair(d,i));
			}
		}
	}
  for(auto &e: adj){
    if(e.size()>MAX_NEIGHBOURS){
      sort(e.begin(),e.end());
      e.erase(e.begin()+MAX_NEIGHBOURS,e.end());
    }
    assert(e.size()<=MAX_NEIGHBOURS);
  }

	puts("Generatin graph done.");

	/**
	 * Control point Distance
	 * Now for each of control point marked by the user, we apply dijkstra algorithm to find shortest path considering control point as source. Thus we evaluate inverse distance for each control point.
	 */
	puts("Calculating the control point distance...");
	vector<vector<float> > cpd(0);
	for(int i=0;i<CONTROL_POINTS;i++){
		cpd.push_back(vector<float>(N+1,FLOAT_MAX));
		dijkstra(cv[i].first,N,adj,cpd[i]);

		for(int j=0;j<=N;j++){
			if(cpd[i][j]==FLOAT_MAX){
				cpd[i][j]=-1;
				continue;
			}
			if(cpd[i][j]>1e-6) cpd[i][j] = 1.0/cpd[i][j];
			else{
				// cout<<"FLOAT_MAX"<<endl;
				cpd[i][j] = FLOAT_MAX;
				// cout<<i<<" "<<j<<endl;
			}
		}
	}
	puts("Done.");

	/**
	 * Distributing the corrosion in the graph
	 * Now from each of control point, we leak to corrosion value along the graph. Leakage is directly proportional to the inverse distance caluculated with dijkstra algorithm. This way we get corrosion value for all the vertices.
	 * Also using this, we get mapping from corrosion value to color.
	 */
	vector<pair<float,Color> > cor_color;
	float max_cor_value=0;
	float deno,num;
	int flag;
	for(int i=0;i<N;i++){
		deno=0;num=0;flag=0;
		for(int j=0;j<CONTROL_POINTS;j++){
			if(cpd[j][i]>=0){
				if(flag!=2)
				flag=1;
				if(cpd[j][i]==FLOAT_MAX){
					flag=2;
					// cout<<j<<" "<<i<<endl;
				}
				deno += cpd[j][i];
				num += cpd[j][i]*cv[j].second;

			}
		}
		if(flag){
			// if(flag==2){
			// 	printf("%f\n",num/deno);
			// }
			cor_color.push_back(mp(num/deno,colors[i]));
		}
	}
	N = cor_color.size();


	int res[10]={0};
	for(int i=0;i<N;i++){
		if(cor_color[i].first<=1.0)
		res[int(cor_color[i].first*10)]++;
	}
	for(int i=0;i<10;i++)
		cout<<res[i]<<" ";
	cout<<endl;

	/**
	 * Generating output
	 * Now for each vertex in the graph, we have corrosion value calculated from control points. Using this corrosion value, and color of corresponding pixel, we generate spectrum of corrosion appearance. This shows appearance at each corrosion degree value.
	 */
	puts("Generating output...");
	Image* out = new Image();

	out->init(100,1000);
  vector<int> blanks(out->h,0);
  vector< vector<Color> > grad_colors(out->h);
  int minh=out->h;
  Color last;
	for(int i=0;i<N;i++){
    int h=int(cor_color[i].first*out->h);
    if(h>=out->h)
      h=out->h-1;
    if(h<0)
      continue;
    if(minh>=h){
      minh=h;
      last=cor_color[i].second;
    }
    grad_colors[h].push_back(cor_color[i].second);
    blanks[h]++;
	}
  puts("Aggregating colors...");
  for(int i=0;i<out->h;i++){
    Color c(0,0,0);
    // if(grad_colors[i].size()>2)
    // {
    c=aggregateColors(grad_colors[i]);
    // }
    printf(".");
    for(int j=0;j<out->w;j++)
      out->set(j,i,c);
  }
  puts("Interpolating unknown corrosion appearances..");
  int last_h=0,next_h=0;
  Color next;
  for(int i=0;i<out->h;i++)
  {
    if(blanks[i]!=0)
    {
      last=out->get(0,i);
      last_h=i;
    }
    else{
      bool no_next=true;
      for(int j=i+1;j<out->h;j++){
        if(blanks[j]!=0){
          next_h=j;
          next=out->get(0,j);
          no_next=false;
          break;
        }
      }
      if(no_next)
        next_h=out->h-1;
      for(;i<next_h;i++){
        float last_w=float(next_h-i+1)/(next_h-last_h-1);
        float next_w=1-last_w;
        // tot=1.f/(last_w+next_w);
        // last_w*=tot;
        // next_w*=tot;
        if(no_next)
          last_w=1,next_w=0;
        for(int j=0;j<out->w;j++){
          // out->set(j,i,Color(0,0,0));
          auto c=last*last_w+next*next_w;
          // auto c=last*last*last_w+next*next*next_w;
          // c.r=sqrt(c.r);
          // c.g=sqrt(c.g);
          // c.b=sqrt(c.b);
          out->set(j,i,c);
          // out->set(j,i,Color(1,1,1)*last_w);
        }
      }
      i--;
    }
  }
  string basename=name.substr(0,name.find_last_of("."));
  cout<<"Base filename: "<<basename<<endl;
  out->save(basename+"-color.bmp");

  Image* wmap = new Image();
	wmap->load(name);
  vector<Color> gradient;
  for(int k=0;k<out->h;k++){
    auto c=out->get(0,k);
    // c.print();
    gradient.push_back(c);
  }
	for(int i=0;i<map->w;i++){
		for(int j=0;j<map->h;j++){
			c = map->get(i,j);
      float min_dist=THRESHOLD2;
      Color nearest=Color(1,0,0);
      for(int k=0;k<gradient.size();k++){
        auto &c1 = gradient[k];
        float d=dist(c1,c);
        if(d<=min_dist){
          min_dist=d;
          nearest=Color(1,1,1)*(float(k)/(out->h-1));
        }
      }
			wmap->set(i,j,nearest);
		}
	}
	wmap->save(basename+"-base.bmp");
  cout<<"Done all";
	if(wmap)delete wmap;
	// Image* hist = new Image();
	// hist->load("weathering_map.bmp");
	// histogramEqualization(hist);
	// hist->save("histogram.bmp");
	// if(hist)delete hist;
	if(map)delete map;
	if(out)delete out;
}

// void histogramEqualization(Image *img){
// 	puts("Histogram Equalization...");
// 	int I[256]={0};
// 	int sum_I[256]={0};
// 	for(int i=0;i<img->w;i++){
// 		for(int j=0;j<img->h;j++){
// 			if(img->get(i,j)==Color(1,0,0))continue;
// 			I[int(img->get(i,j).r*255)]++;
// 		}
// 	}
// 	sum_I[0]=I[0];
// 	for(int i=1;i<256;i++)
// 		sum_I[i]=sum_I[i-1]+I[i];

// 	float new_I[256]={0};
// 	for(int i=0;i<256;i++){
// 		new_I[i] = (float(sum_I[i])/sum_I[255]);
// 	}
// 	int _I;
// 	float _new_I;
// 	for(int i=0;i<img->w;i++){
// 		for(int j=0;j<img->h;j++){
// 			if(img->get(i,j)==Color(1,0,0))continue;
// 			_I = img->get(i,j).r*255;
// 			_new_I=new_I[_I];
// 			img->set(i,j,Color(_new_I,_new_I,_new_I));
// 		}
// 	}
// 	puts("Done.");
// }