#include "par.h"
#include "su.h"
#include "segy.h"
#include "omp.h"
#define BUFSIZE 1000
char *sdoc[] = {

" This program will read *.msh generated by Gmsh. ",
" msh2tri < *.msh [parameters] ",
" ALL characters and the number of nodes and elements should be delected before reading. ",
" mele	number of columns of elements. ",
" p q r	three criterion for finding ture elements. ",
NULL};

int main(int argc, char **argv)
{	
	char buf[BUFSIZE];
	int i,j,k;
	int nnd,nele,ntele;
	int mnd,mele;
	int nline;
	int *linenumber;
	float p,q,r;
	float l1,l2,l3;
	int flag=0;
	int *lflag;
	float *elea;
	float **nd;
	float **ele;
	float **linecoo;
	int **neigh;
	int sumline;
	int np,ip;
	int d1,d2,d3;
	int tmpid;
	
	FILE *mshfp=stdin;
	FILE *ndfp;
	FILE *elefp;
	FILE *linefp;
	FILE *neighfp;

	initargs(argc,argv);
	requestdoc(0);

	if(!getparint("mnd",&mnd)) mnd=4;
	if(!getparint("mele",&mele)) err("no mele!!!, 8 for 1-order 2D, 9 for 1-order 3D");
	if(!getparfloat("p",&p)) err("must specify p: 2 -> 2D, 3 -> 3D.");
	if(!getparfloat("q",&q)) q=2;
	if(!getparfloat("r",&r)) r=0;
	if(!getparfloat("l1",&l1)) l1=1;
	if(!getparfloat("l2",&l2)) l2=2;
	if(!getparfloat("l3",&l3)) l3=0;
 
	nline = countparval("linenumber");
	if (nline!=0) {
		linenumber=alloc1int(nline);
		lflag=alloc1int(nline);
		getparint("linenumber",linenumber);
	}
	
	fgets(buf,BUFSIZE,mshfp);
	fgets(buf,BUFSIZE,mshfp);
	fgets(buf,BUFSIZE,mshfp);
	fgets(buf,BUFSIZE,mshfp);
	fscanf(mshfp,"%d", &nnd);
	warn("nnd=%d",nnd);
	
	nd = alloc2float(mnd,nnd);

	warn("Open the mesh file");
	warn("Reading nodes");
	/* read nodes */	
	for(i=0;i<nnd;i++)
		for(j=0;j<mnd;j++)
		{	
			fscanf(mshfp,"%f",&nd[i][j]);
		}
	warn("Done reading nodes");
	
	/* read elements */
	fgets(buf,BUFSIZE,mshfp);
	fgets(buf,BUFSIZE,mshfp);
	fgets(buf,BUFSIZE,mshfp);
	fscanf(mshfp,"%d", &nele);
	warn("nele=%d",nele);
	elea = alloc1float(nele*mele);
	
	
	for(i=0;i<nele*mele;i++)
	{
		elea[i] = -1.0;
		if(feof(mshfp)) {break;warn("end elements");}
		else {fscanf(mshfp,"%f",&elea[i]);}
	}
	warn("Done reading elements");
	warn("Now translating");
	/* count number of elements and lines */
	for(i=0,flag=0;i<nele*mele-2;i++)
	{
		if(elea[i]==p&&elea[i+1]==q&&elea[i+2]==r) flag++;
		for (j=0;j<nline;++j)
			if(elea[i]==l1&&elea[i+1]==l2&&elea[i+2]==l3&&elea[i+3]==linenumber[j]) 
				lflag[j]++;
	}
	/* display the counting result */
	warn("Done counting!Triangle elements:%d",flag);
	ntele = flag;
	ele =alloc2float(5,ntele);
	if(nline!=0) {
		for(i=0,sumline=0;i<nline;i++) {
			warn("linenumber=%d,there are %d.",linenumber[i],lflag[i]);
			sumline=sumline+lflag[i];
		}
				
		warn("line-elements :%d",sumline);sumline++;
		warn("output lines.bin nline :%d",sumline);
		linecoo=alloc2float(2,sumline);
	}
	
	/* from elea to linecoo */
	if(nline!=0) {	
		for(i=0,flag=0;i<nele*mele-3;i++)
		{
			if(elea[i]==-1.0) break;
			for (j=0;j<nline;++j)
				if(elea[i]==l1&&elea[i+1]==l2&&elea[i+2]==l3&&elea[i+3]==linenumber[j]) {
					linecoo[flag][0]=nd[(int)elea[i+4]-1][1];
					linecoo[flag][1]=nd[(int)elea[i+4]-1][2];
					if(flag==(sumline-2)) {
						linecoo[flag+1][0]=nd[(int)elea[i+5]-1][1];
						linecoo[flag+1][1]=nd[(int)elea[i+5]-1][2];
					}
					flag++;
				}
		}
		if ((linefp = fopen("lines.bin","w"))==NULL)
			err("Can't write file!");
		else {
			fwrite(linecoo[0],sizeof(float),sumline*2,linefp);
			fclose(linefp);
			warn("Done writing lines.bin,%d points on lines.",flag+1);
		}
	}
	/* from elea to ele */
	flag=0;
	for(i=0,flag=0;i<nele*mele-(mele-2);i++)
	{
		if(elea[i]==-1.0) break;
		if(elea[i]==p&&elea[i+1]==q&&elea[i+2]==r)
		{
			ele[flag][0]=flag+1;
			for (j=1;j<4;j++)
				ele[flag][j]=elea[i+j+3];
			ele[flag][4]=elea[i+3];
			flag++;
		}
	}
	
	
	for (i=0,np=1,ip=ele[0][4];i<ntele;i++) {
		if (ele[i][4]!=np && ele[i][4]!=ip) {
			ip=ele[i][4];
			np++;
			ele[i][4]=np;
		}
		else if (ele[i][4]!=np && ele[i][4]==ip) {
			ip=ele[i][4];
			ele[i][4]=np;
		}
			
	}
	warn("Plane Surface number: %d",np);
	
	warn("there are %d nodes(see nodes.bin),and %d elements(see elements.bin).%d points on lines(see lines.bin).",nnd,flag,sumline);
	free1float(elea);
	warn("Done translating!");
	fclose(mshfp);
	
	
	/* generating neigh info */
	warn("Now, reorganizing neigh");
	neigh = alloc2int(4,ntele);
#pragma omp parallel for \
private(i,j,k,d1,d2,d3) \
num_threads(16)
	for (i=0;i<ntele;i++) {
		neigh[i][0] = ele[i][0];
		d1 = ele[i][1];
		d2 = ele[i][2];
		d3 = ele[i][3];
		neigh[i][1]=-1;
		neigh[i][2]=-1;
		neigh[i][3]=-1;

		for (j=0;j<ntele;j++) {
			if (j!=i) {
				for (k=0;k<3;k++) {
					if ( (ele[j][k%3+1]==d1 && ele[j][(k+1)%3+1]==d2) || (ele[j][(k+1)%3+1]==d1 && ele[j][k%3+1]==d2) ) {
						neigh[i][1] = j+1;
						break;
					}
					if ( (ele[j][k%3+1]==d2 && ele[j][(k+1)%3+1]==d3) || (ele[j][(k+1)%3+1]==d2 && ele[j][k%3+1]==d3) ) {
						neigh[i][2] = j+1;
						break;
					}
					if ( (ele[j][k%3+1]==d1 && ele[j][(k+1)%3+1]==d3) || (ele[j][(k+1)%3+1]==d3 && ele[j][k%3+1]==d1) ) {
						neigh[i][3] = j+1;
						break;
					}
				}
			}
		}
		
		if (i%1000==0)
			warn("done ele:%d",i+1);
	}
	
	/* delete unused nodes */ 
	int flag1=0;
	int tofind;
	int use;
	float **nd_t;
	int nnd_t=nnd;
	nd_t = alloc2float(mnd,nnd);
	warn("Now, deleting unused nodes");
	for (i=0,flag1=0;i<nnd;i++) {
		tofind = i+1;
		for (j=0,use=0;j<ntele;j++) {
			if (ele[j][1]==tofind) {
				if (use==0) {
					use = 1;flag1++;
					nd_t[flag1-1][0]=flag1;
					nd_t[flag1-1][1]=nd[i][1];
					nd_t[flag1-1][2]=nd[i][2];
					nd_t[flag1-1][3]=nd[i][3];
				}
				
				ele[j][1]=flag1;
				continue;
			}
			if (ele[j][2]==tofind) {
				if (use==0) {
					use = 1;flag1++;
					nd_t[flag1-1][0]=flag1;
					nd_t[flag1-1][1]=nd[i][1];
					nd_t[flag1-1][2]=nd[i][2];
					nd_t[flag1-1][3]=nd[i][3];
				}
				
				ele[j][2]=flag1;
				continue;
			}
			if (ele[j][3]==tofind) {
				if (use==0) {
					use = 1;flag1++;
					nd_t[flag1-1][0]=flag1;
					nd_t[flag1-1][1]=nd[i][1];
					nd_t[flag1-1][2]=nd[i][2];
					nd_t[flag1-1][3]=nd[i][3];
				}
				
				ele[j][3]=flag1;
				continue;
			}
		}
		if (use==0) {
			nnd_t--;
		}
		if (i%1000==0)
			warn("done node:%d, use? %d",i+1,use);
	}
	warn("nnd_t=%d",nnd_t);
	

		
	/* writing nodes file and elements file */
	/*******************************************/

	if ((ndfp = fopen("nodes.1.node","w"))==NULL)
		err("Can't write file!");
	else {
		fprintf(ndfp,"%d 2 0 0\n",nnd_t);
		for (i=0;i<nnd_t;i++)
			fprintf(ndfp,"%d %.3f %.3f 0\n",(int)nd_t[i][0],nd_t[i][1],nd_t[i][2]);
		fclose(ndfp);
		warn("Done writing nodes");
	}
	
	if ((elefp = fopen("elements.1.ele","w"))==NULL)
		err("Can't write file!");
	else {
		fprintf(elefp,"%d 3 %d\n",ntele,np);
		for (i=0;i<ntele;i++)
			fprintf(elefp,"%d %d %d %d %d\n",
				(int)ele[i][0],(int)ele[i][1],(int)ele[i][2],
				(int)ele[i][3],(int)ele[i][4]);
		fclose(elefp);
		warn("Done writing elements");
	}
	
	if ((neighfp = fopen("neigh.1.neigh","w"))==NULL)
		err("Can't write file!");
	else {
		fprintf(neighfp,"%d 3\n",ntele);
		for (i=0;i<ntele;i++)
			fprintf(neighfp,"%d %d %d %d\n",neigh[i][0],neigh[i][1],
				neigh[i][2],neigh[i][3]);
		fclose(neighfp);
		warn("Done writing neigh");
	}
	warn("Done writing all files!");
	
	warn("********* INFO *********");
	warn("number of nodes: %d",nnd);
	warn("number of elements: %d",ntele);
	warn("Plane Surface number: %d",np);
	warn("1st node %g",nd[0][0]);
	warn("2nd node %g",nd[1][0]);
	warn("1st element %g",ele[0][0]);
	warn("2nd element %g",ele[1][0]);
	warn("*****************************");
	free2float(nd);
	free2float(ele);
	free2int(neigh);
	if(nline!=0) free2float(linecoo);
	return 0;
}
