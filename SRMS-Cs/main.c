/*
 SRMS v3.2 - Clean UI Edition (ASCII) — FIXED totals & percentages
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined(_WIN32) || defined(_WIN64)
  #include <conio.h>
  #include <windows.h>
  #define GETCH() _getch()
  #define SLEEP(ms) Sleep(ms)
  #define strcasecmp _stricmp
#else
  #include <termios.h>
  #include <unistd.h>
  static int getch_unix(void){
    struct termios oldt,newt; int ch;
    tcgetattr(0,&oldt); newt=oldt; newt.c_lflag&=~(ICANON|ECHO);
    tcsetattr(0,TCSANOW,&newt); ch=getchar();
    tcsetattr(0,TCSANOW,&oldt); return ch;
  }
  #define GETCH() getch_unix()
  #define SLEEP(ms) usleep((ms)*1000)
#endif

/* ====================== CONFIG ====================== */
#define DB_FILE "students.dat"
#define LOGIN_FILE "credentials.txt"
#define NAME_SIZE 64
#define NUM_SUBS 7

const char *SUB_NAMES[NUM_SUBS] = {
  "Discrete Mathematics","Digital Electronics","OOPS Theory",
  "DAA Theory","Open Elective","Coding Skills","Problem Solving"
};
const int HAS_LAB[NUM_SUBS] = {0,0,1,1,0,0,0};

/* ====================== STRUCT ====================== */
typedef struct {
  int roll;
  char name[NAME_SIZE];
  float marks[NUM_SUBS][2]; /* [][0]=theory, [][1]=lab or -1 */
  int attended[NUM_SUBS];
  int total_classes[NUM_SUBS];
} Student;

char currentUser[64]="";
char currentRole[32]="";

/* ====================== UI ====================== */
void print_main_header(){
  printf("\n=================== STUDENT RECORD MANAGEMENT SYSTEM ===================\n\n");
}

void print_section_header(const char *title){
  printf("\n------------------------- %s -------------------------\n", title);
}

void wait_key(){
  printf("\nPress any key to continue...");
  GETCH();
  printf("\n");
}

/* ====================== INPUT HELPERS ====================== */
void read_line(char *buf,int n){
  if (!fgets(buf,n,stdin)) { buf[0]=0; return; }
  buf[strcspn(buf,"\n")] = 0;
}

char *getPassword(char *buf,int n){
  int i=0; char ch;
  while(1){
    ch=GETCH();
    if(ch=='\r'||ch=='\n'){ buf[i]=0; printf("\n"); break; }
    if((ch==127||ch==8) && i>0){ i--; printf("\b \b"); continue; }
    if(isprint(ch) && i<n-1){ buf[i++]=ch; printf("*"); }
  }
  return buf;
}

/* ====================== STORAGE ====================== */
Student* load_all(int *count){
  *count=0;
  FILE *f=fopen(DB_FILE,"rb");
  if(!f) return NULL;
  fseek(f,0,SEEK_END);
  long sz=ftell(f);
  if(sz<=0){ fclose(f); return NULL; }
  rewind(f);
  *count = (int)(sz / sizeof(Student));
  Student *arr = malloc((*count) * sizeof(Student));
  if(!arr){ fclose(f); *count = 0; return NULL; }
  if(fread(arr,sizeof(Student),*count,f) != (size_t)(*count)){ free(arr); fclose(f); *count = 0; return NULL; }
  fclose(f);
  return arr;
}

int save_all(Student *arr,int count){
  FILE *f=fopen(DB_FILE,"wb");
  if(!f) return 0;
  if(count>0) fwrite(arr,sizeof(Student),count,f);
  fclose(f);
  return 1;
}

/* ====================== UTIL ====================== */
int roll_exists(int r){
  int n=0; Student *a=load_all(&n);
  if(!a) return 0;
  for(int i=0;i<n;i++){
    if(a[i].roll==r){ free(a); return 1; }
  }
  free(a);
  return 0;
}

const char* grade(float p){
  if(p>=90) return "A+";
  if(p>=80) return "A";
  if(p>=70) return "B";
  if(p>=60) return "C";
  if(p>=50) return "D";
  return "F";
}

/* ====================== FIXED TOTALS & PCT ====================== */
/* Include only valid (>=0) marks in totals and corresponding max.
   Theory considered 100 if a theory mark is provided (>=0).
   Lab considered 100 only if HAS_LAB and lab mark >=0.
*/
void compute_totals(const Student *s, float *tot, float *maxp, float *pct){
  float t=0.0f, m=0.0f;
  for(int i=0;i<NUM_SUBS;i++){
    /* Theory */
    float th = s->marks[i][0];
    if(th >= 0.0f){ /* include theory only if non-negative */
      if(th > 100.0f) th = 100.0f; /* clamp */
      t += th;
      m += 100.0f;
    }
    /* Lab */
    if(HAS_LAB[i]){
      float lb = s->marks[i][1];
      if(lb >= 0.0f){ /* include lab only if non-negative */
        if(lb > 100.0f) lb = 100.0f;
        t += lb;
        m += 100.0f;
      }
    }
  }
  *tot = t; *maxp = m;
  *pct = (m > 0.0f) ? (t / m * 100.0f) : 0.0f;
}

/* ====================== PRINT STUDENT ====================== */
void print_student_brief(const Student *s){
  float tot,max,p; compute_totals(s,&tot,&max,&p);
  printf("Roll: %-5d | Name: %-20s | %6.1f / %-6.1f (%5.2f%%) | Grade: %s\n",
        s->roll, s->name, tot, max, p, grade(p));
}

/* ====================== CORE OPS ====================== */
void add_student(){
  Student s; memset(&s,0,sizeof(s));
  for(int i=0;i<NUM_SUBS;i++){ s.marks[i][0]=0.0f; s.marks[i][1] = (HAS_LAB[i] ? 0.0f : -1.0f); s.attended[i]=0; s.total_classes[i]=0; }

  print_section_header("ADD STUDENT");
  printf("Enter roll: "); if(scanf("%d",&s.roll)!=1){ while(getchar()!='\n'); printf("Bad input.\n"); return; } getchar();

  if(roll_exists(s.roll)){ printf("Roll already exists.\n"); return; }

  printf("Enter full name: "); read_line(s.name,NAME_SIZE);

  for(int i=0;i<NUM_SUBS;i++){
    printf("%s Theory (enter -1 to skip): ", SUB_NAMES[i]); if(scanf("%f",&s.marks[i][0])!=1){ while(getchar()!='\n'); printf("Bad input.\n"); return; } getchar();
    if(HAS_LAB[i]){
      printf("%s Lab (enter -1 to skip): ", SUB_NAMES[i]); if(scanf("%f",&s.marks[i][1])!=1){ while(getchar()!='\n'); printf("Bad input.\n"); return; } getchar();
    } else s.marks[i][1] = -1.0f;

    printf("%s Attended (enter 0 if none): ", SUB_NAMES[i]); if(scanf("%d",&s.attended[i])!=1){ while(getchar()!='\n'); printf("Bad input.\n"); return; } getchar();
    printf("%s Total Classes (enter 0 if none): ", SUB_NAMES[i]); if(scanf("%d",&s.total_classes[i])!=1){ while(getchar()!='\n'); printf("Bad input.\n"); return; } getchar();
  }

  int n=0; Student *arr = load_all(&n);
  Student *newarr = malloc((n+1)*sizeof(Student));
  if(n>0) memcpy(newarr,arr,n*sizeof(Student));
  newarr[n] = s;
  if(save_all(newarr,n+1)) printf("\nStudent added successfully.\n"); else printf("\nSave failed.\n");
  free(arr); free(newarr);
}

void display_students(){
  int n=0; Student *a=load_all(&n);
  print_section_header("STUDENT LIST");
  if(!a || n==0){ printf("No records.\n"); return; }

  for(int i=0;i<n;i++){
    print_student_brief(&a[i]);
    printf("------------------------------------------------------------\n");
    printf("%-25s %-8s %-8s %-18s\n","Subject","Theory","Lab","Attendance");
    printf("------------------------------------------------------------\n");

    for(int j=0;j<NUM_SUBS;j++){
      float th = a[i].marks[j][0];
      float lb = a[i].marks[j][1];
      int att = a[i].attended[j];
      int totc = a[i].total_classes[j];
      float pct = (totc>0) ? (att*100.0f / totc) : 0.0f;

      char labbuf[8];
      if(HAS_LAB[j]){
        if(lb < 0.0f) strcpy(labbuf," - ");
        else sprintf(labbuf,"%.1f", (lb>100.0f?100.0f:lb));
      } else strcpy(labbuf," - ");

      char thbuf[16];
      if(th < 0.0f) strcpy(thbuf," - ");
      else sprintf(thbuf,"%.1f", (th>100.0f?100.0f:th));

      printf("%-25s %8s %8s %3d/%-6d (%5.1f%%)\n",
           SUB_NAMES[j], thbuf, labbuf, att, totc, pct);
    }
    printf("------------------------------------------------------------\n");
  }
  free(a);
}

void search_student(){
  int n=0; Student *a=load_all(&n);
  print_section_header("SEARCH STUDENT");
  if(!a || n==0){ printf("No records.\n"); return; }

  printf("1) Search by Roll\n2) Search by Name\nChoice: ");
  int c; if(scanf("%d",&c)!=1){ while(getchar()!='\n'); free(a); return; } getchar();

  if(c==1){
    int r; printf("Enter roll: "); if(scanf("%d",&r)!=1){ while(getchar()!='\n'); free(a); return; } getchar();
    for(int i=0;i<n;i++){
      if(a[i].roll==r){
        print_student_brief(&a[i]);
        free(a); return;
      }
    }
    printf("Not found.\n");
  } else if(c==2){
    char name[NAME_SIZE]; printf("Enter name: "); read_line(name,NAME_SIZE);
    for(int i=0;i<n;i++){
      if(strcasecmp(name,a[i].name)==0){
        print_student_brief(&a[i]);
        free(a); return;
      }
    }
    printf("Not found.\n");
  } else printf("Invalid choice.\n");

  free(a);
}

void update_student(){
  int n=0; Student *a=load_all(&n);
  print_section_header("UPDATE STUDENT");
  if(!a||n==0){ printf("No records.\n"); return; }

  printf("Enter roll to update: "); int r; if(scanf("%d",&r)!=1){ while(getchar()!='\n'); free(a); return; } getchar();
  int idx=-1; for(int i=0;i<n;i++) if(a[i].roll==r) idx=i;
  if(idx==-1){ printf("Student not found.\n"); free(a); return; }

  printf("Current name: %s\nNew name (enter to keep): ", a[idx].name); char nm[NAME_SIZE]; read_line(nm,NAME_SIZE); if(strlen(nm)) strcpy(a[idx].name,nm);

  for(int j=0;j<NUM_SUBS;j++){
    float v;
    printf("%s Theory [%.1f] (-1 keep): ",SUB_NAMES[j],a[idx].marks[j][0]);
    if(scanf("%f",&v)!=1){ while(getchar()!='\n'); free(a); return; } getchar(); if(v>=0) a[idx].marks[j][0] = (v>100?100:v);

    if(HAS_LAB[j]){
      printf("%s Lab [%.1f] (-1 keep): ",SUB_NAMES[j],a[idx].marks[j][1]);
      if(scanf("%f",&v)!=1){ while(getchar()!='\n'); free(a); return; } getchar(); if(v>=0) a[idx].marks[j][1] = (v>100?100:v);
    }

    printf("%s Attended [%d] (-1 keep): ",SUB_NAMES[j],a[idx].attended[j]); int at; if(scanf("%d",&at)!=1){ while(getchar()!='\n'); free(a); return; } getchar(); if(at>=0) a[idx].attended[j]=at;
    printf("%s TotalClasses [%d] (-1 keep): ",SUB_NAMES[j],a[idx].total_classes[j]); int tc; if(scanf("%d",&tc)!=1){ while(getchar()!='\n'); free(a); return; } getchar(); if(tc>=0) a[idx].total_classes[j]=tc;
  }

  if(save_all(a,n)) printf("Updated.\n"); else printf("Save failed.\n");
  free(a);
}

void delete_student(){
  int n=0; Student *a=load_all(&n);
  print_section_header("DELETE STUDENT");
  if(!a||n==0){ printf("No records.\n"); return; }

  printf("Enter roll: "); int r; if(scanf("%d",&r)!=1){ while(getchar()!='\n'); free(a); return; } getchar();
  int idx=-1; for(int i=0;i<n;i++) if(a[i].roll==r) idx=i;
  if(idx==-1){ printf("Student not found.\n"); free(a); return; }

  printf("Confirm delete (y/n): "); char c=GETCH(); getchar();
  if(c!='y' && c!='Y'){ printf("Cancelled.\n"); free(a); return; }

  for(int i=idx;i<n-1;i++) a[i]=a[i+1];
  if(save_all(a,n-1)) printf("Deleted.\n"); else printf("Delete failed.\n");
  free(a);
}

void update_marks_staff(){
  int n=0; Student *a=load_all(&n);
  print_section_header("STAFF: UPDATE MARKS & ATTENDANCE");
  if(!a||n==0){ printf("No records.\n"); return; }

  printf("Enter roll: "); int r; if(scanf("%d",&r)!=1){ while(getchar()!='\n'); free(a); return; } getchar();
  int idx=-1; for(int i=0;i<n;i++) if(a[i].roll==r) idx=i;
  if(idx==-1){ printf("Not found.\n"); free(a); return; }

  for(int j=0;j<NUM_SUBS;j++){
    float v;
    printf("%s Theory [%.1f] (-1 keep): ",SUB_NAMES[j],a[idx].marks[j][0]);
    if(scanf("%f",&v)!=1){ while(getchar()!='\n'); free(a); return; } getchar(); if(v>=0) a[idx].marks[j][0] = (v>100?100:v);

    if(HAS_LAB[j]){
      printf("%s Lab [%.1f] (-1 keep): ",SUB_NAMES[j],a[idx].marks[j][1]);
      if(scanf("%f",&v)!=1){ while(getchar()!='\n'); free(a); return; } getchar(); if(v>=0) a[idx].marks[j][1] = (v>100?100:v);
    }

    printf("%s Attended [%d] (-1 keep): ",SUB_NAMES[j],a[idx].attended[j]); int at; if(scanf("%d",&at)!=1){ while(getchar()!='\n'); free(a); return; } getchar(); if(at>=0) a[idx].attended[j]=at;
    printf("%s Total Classes [%d] (-1 keep): ",SUB_NAMES[j],a[idx].total_classes[j]); int tc; if(scanf("%d",&tc)!=1){ while(getchar()!='\n'); free(a); return; } getchar(); if(tc>=0) a[idx].total_classes[j]=tc;
  }

  if(save_all(a,n)) printf("Updated.\n"); else printf("Save failed.\n");
  free(a);
}

void export_report(){
  int n=0; Student *a=load_all(&n);
  print_section_header("EXPORT REPORT");
  if(!a||n==0){ printf("No records.\n"); return; }

  printf("Enter roll: "); int r; if(scanf("%d",&r)!=1){ while(getchar()!='\n'); free(a); return; } getchar();

  for(int i=0;i<n;i++){
    if(a[i].roll==r){
      char fname[64]; sprintf(fname,"report_%d.txt",r);
      FILE *f=fopen(fname,"w");
      if(!f){ printf("Failed to create file.\n"); free(a); return; }

      float tot,max,p; compute_totals(&a[i],&tot,&max,&p);
      fprintf(f,"STUDENT REPORT\nRoll: %d\nName: %s\n\n",a[i].roll,a[i].name);
      fprintf(f,"%-25s %-6s %-6s %-14s\n","Subject","T","L","Attendance");

      for(int j=0;j<NUM_SUBS;j++){
        float pct = a[i].total_classes[j] ? a[i].attended[j]*100.0f/a[i].total_classes[j] : 0.0f;
        if(HAS_LAB[j]){
          if(a[i].marks[j][1] < 0.0f)
            fprintf(f,"%-25s %6.1f %6s %2d/%-3d (%5.1f%%)\n", SUB_NAMES[j], (a[i].marks[j][0]<0?0:a[i].marks[j][0]), "-", a[i].attended[j], a[i].total_classes[j], pct);
          else
            fprintf(f,"%-25s %6.1f %6.1f %2d/%-3d (%5.1f%%)\n", SUB_NAMES[j], (a[i].marks[j][0]<0?0:a[i].marks[j][0]), a[i].marks[j][1], a[i].attended[j], a[i].total_classes[j], pct);
        } else {
          fprintf(f,"%-25s %6.1f %6s %2d/%-3d (%5.1f%%)\n", SUB_NAMES[j], (a[i].marks[j][0]<0?0:a[i].marks[j][0]), "-", a[i].attended[j], a[i].total_classes[j], pct);
        }
      }

      fprintf(f,"\nTotal: %.1f / %.1f\nPercentage: %.2f%%\nGrade: %s\n", tot, max, p, grade(p));
      fclose(f);
      printf("Report exported: %s\n",fname);
      free(a);
      return;
    }
  }

  printf("Student not found.\n");
  free(a);
}

/* ====================== LOGIN & MENUS ====================== */
int login_system(){
  print_section_header("LOGIN");

  char u[64], p[64];
  printf("Username: "); read_line(u,64);
  printf("Password: "); getPassword(p,64);

  FILE *f=fopen(LOGIN_FILE,"r");
  if(!f){ printf("Missing credentials.txt\n"); return 0; }

  char fu[64], fp[64], fr[32];
  while(fscanf(f,"%s %s %s",fu,fp,fr)==3){
    if(strcmp(u,fu)==0 && strcmp(p,fp)==0){
      strcpy(currentUser,fu); strcpy(currentRole,fr); fclose(f);
      printf("\nLogin successful.\n"); return 1;
    }
  }

  fclose(f);
  printf("\nInvalid credentials.\n");
  return 0;
}

void admin_menu(){
  while(1){
    print_main_header();
    print_section_header("ADMIN PANEL");
    printf("1) Add Student\n2) Display Students\n3) Search Student\n4) Update Student\n");
    printf("5) Delete Student\n6) Export Report\n7) Logout\n");
    printf("Choice: ");
    int c; if(scanf("%d",&c)!=1){ while(getchar()!='\n'); c=-1; } getchar();
    if(c==1) add_student();
    else if(c==2) display_students();
    else if(c==3) search_student();
    else if(c==4) update_student();
    else if(c==5) delete_student();
    else if(c==6) export_report();
    else if(c==7) return;
    wait_key();
  }
}

void staff_menu(){
  while(1){
    print_main_header();
    print_section_header("STAFF PANEL");
    printf("1) Display Students\n2) Search Student\n3) Update Marks/Attendance\n4) Export\n5) Logout\n");
    printf("Choice: ");
    int c; if(scanf("%d",&c)!=1){ while(getchar()!='\n'); c=-1; } getchar();
    if(c==1) display_students();
    else if(c==2) search_student();
    else if(c==3) update_marks_staff();
    else if(c==4) export_report();
    else if(c==5) return;
    wait_key();
  }
}

void user_menu(){
  while(1){
    print_main_header();
    print_section_header("USER PANEL");
    printf("1) Display Students\n2) Search Student\n3) Logout\nChoice: ");
    int c; if(scanf("%d",&c)!=1){ while(getchar()!='\n'); c=-1; } getchar();
    if(c==1) display_students();
    else if(c==2) search_student();
    else if(c==3) return;
    wait_key();
  }
}

void guest_menu(){
  while(1){
    print_main_header();
    print_section_header("GUEST PANEL");
    printf("1) Student Count\n2) Logout\nChoice: ");
    int c; if(scanf("%d",&c)!=1){ while(getchar()!='\n'); c=-1; } getchar();
    if(c==1){
      int n=0; Student *a=load_all(&n);
      printf("\nTotal Students: %d\n",n);
      free(a);
    }
    else if(c==2) return;
    wait_key();
  }
}

/* ====================== MAIN ====================== */
int main(){
  print_main_header();
  if(!login_system()){
    printf("\nAccess denied.\n");
    return 0;
  }
  if(strcmp(currentRole,"ADMIN")==0) admin_menu();
  else if(strcmp(currentRole,"STAFF")==0) staff_menu();
  else if(strcmp(currentRole,"GUEST")==0) guest_menu();
  else user_menu();
  printf("\nThank you. Goodbye.\n");
  return 0;
}
