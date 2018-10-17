/*
まず実装するもの：
  構造体で各種オブジェクトを用意する。 ok
  フィールドを初期化して地雷を一定の割合で置き、シャッフルする。 ok
  キーボード入力から開けるマスを座標式でユーザに選択させ、その後開けたタイルの地雷の有無で 
  ゲームを終わったりタイルを可視化したりする。 ok
次に実装するもの：
  特定の入力で地雷がありそうであることを意味する旗を立てられるようにする。 ok
  特定の入力で旗の数と数字が一致しているマスの周囲を開けられるようにする。 ok
そのうち実装したいもの：
  コマンドライン等から縦横サイズを取得し、動的にマップサイズをとる。 ok
  マップ上でカーソルを動かして操作できるようにする。
  難易度を分ける。(動的なマップサイズ確保と関連) ok
  クリアタイムのファイル保存 ok

  
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>

//真偽値
#define TRUE 1
#define FALSE 0

//mapsize上限
#define MAX_WIDTH 50
#define MAX_HEIGHT 50


//タイルの構造体。地雷があるかどうかと表示状態、周囲の地雷数を持つ。
typedef struct {
  int state;
  int is_mine;
  int around_mines;
}tile_t;

//マップの構造体。
typedef struct {
  int width;
  int height;
  int total_mine;
  tile_t** field_p;
  int difficulty;
}map_t;

void set_map(map_t* map_p,int argc,char* argv[]);
void init_map(map_t* map_p);
void apply_size(int argc,char* argv[],map_t* map_p);
void malloc_field(map_t* map_p);
void set_difficulty(map_t* map_p,int diff);
void set_size_by_difficulty(map_t* map_p);
void set_size(map_t* map_p,char* argv[]);
void show_usage(void);
void swap_tile(tile_t* a,tile_t* b);
void play(map_t* map_p);
void show_field(map_t* map_p);
char select_letter(tile_t** field_p,int x,int y);
int open_tile(map_t* map_p,int is_first);
int get_input(int* x,int* y,map_t* map_p);
int get_parameter(char buf[255],char* ident,int* tmp_x,int* tmp_y,map_t* map_p);
int judge_ident(char ident);
int check_string(char buf[255]);
void set_mines(map_t* map_p,int exception_x,int exception_y);
void shuffle_field(map_t* map_p,int exception_x,int exception_y);
int open_one(map_t* map_p,int x,int y);
void put_flag(map_t* map_p,int x,int y);
int open_around(map_t* map_p,int x,int y);
void put_quest(map_t* map_p,int x,int y);
int count_around_mines(map_t* map_p,int x,int y);
int count_around_flags(map_t* map_p,int x,int y);
void chain(map_t* map_p,int x,int y);
int judge_cleared(map_t* map_p);
void game_over(int is_cleared,int time,map_t* map_p);
void create_result(int time,map_t* map_p);
int get_high_score(FILE* fp);
void write_date(FILE* fp);
int write_score(FILE* fp,int time,map_t* map_p);
void free_field(map_t* map_p);


//タイルの表示状態に関する列挙体。
enum STATE{HIDDEN,SHOWN,FLAGED,QUEST};

//タイルの操作タイプに関する列挙体。
enum OPEN{OPEN,FLAG,AROUND};

//難易度に関する列挙体。
enum DIFFICULTY{EASY=1,NORMAL,HARD};

//大きい関数を呼び出したり、main関数でしかできないことだけをやる
int main(int argc,char* argv[])
{
  map_t map;

  srand((unsigned)time(NULL));
  
  set_map(&map,argc,argv);

  play(&map);

  free_field(&map);
  
  return 0;
}


// マップ関係のマスター関数
// 元はMの設置も行っていたが、機能の改善により移動
void set_map(map_t* map_p,int argc,char* argv[])
{
  apply_size(argc,argv,map_p);

  init_map(map_p);
}

//コマンドラインから属性を取り出し、適用する関数 一部エラー処理を行う
void apply_size(int argc,char* argv[],map_t* map_p)
{

  //コマンドライン引数の数を確認し、見合った処理を行う
  if(argc==2){
    set_difficulty(map_p,atoi(argv[1]));
    set_size_by_difficulty(map_p);
  }else if(argc==4){
    set_size(map_p,argv);
    map_p->difficulty=0;
  }else{
    show_usage();
  }

  malloc_field(map_p);
  
}

//マップを動的に確保する関数、エラー処理つき
void malloc_field(map_t* map_p)
{
  int i;
  
  map_p->field_p=(tile_t**)malloc(sizeof(tile_t*)*map_p->height);
  if(map_p->field_p==NULL){
    printf("failed to generate map.\n");
    exit(EXIT_FAILURE);
  }
  for(i=0;i<map_p->height;i++){
    map_p->field_p[i]=(tile_t*)malloc(sizeof(tile_t)*map_p->width);
    if(map_p->field_p[i]==NULL){
      printf("failed to generate map.\n");
      exit(EXIT_FAILURE);
    }
  }

}

//コマンドラインから難易度を指定された際に難易度を設定し、異常があればエラー処理をする関数
void set_difficulty(map_t* map_p,int diff)
{
  switch(diff){
  case EASY:
  case NORMAL:
  case HARD:
    map_p->difficulty=diff;
    break;
  default:
    show_usage();
  }
}

//コマンドラインから難易度を指定された際のマップサイズ設定関数
void set_size_by_difficulty(map_t* map_p)
{
  switch(map_p->difficulty){
  case EASY:
    map_p->width=8;
    map_p->height=8;
    map_p->total_mine=10;
    break;
  case NORMAL:
    map_p->width=15;
    map_p->height=10;
    map_p->total_mine=35;
    break;
  case HARD:
    map_p->width=20;
    map_p->height=18;
    map_p->total_mine=100;
    break;
  }
  
}

//ユーザが任意にサイズを入力した際のマップサイズ設定関数
void set_size(map_t* map_p,char* argv[])
{
  int width,height,mines;

  //コマンドライン引数を整数データ化
  width=atoi(argv[1]);
  height=atoi(argv[2]);
  mines=atoi(argv[3]);

  if(width<=0 || width>MAX_WIDTH || height<=0 || height>MAX_WIDTH || mines<=0 || mines>width*height){
    show_usage();
  }
  if(width==1 && height==1){
    show_usage();
  }

  map_p->width=width;
  map_p->height=height;
  map_p->total_mine=mines;
  
}

//エラー処理として異常な入力に対してユーザに正常な入力法を伝える関数
void show_usage(void)
{
  printf("usage:\n$ ./msweeper (difficulty[1,2,3])\nor\n$ ./msweeper (map width<50) (map height<50) (number of mines)\n");
  exit(EXIT_SUCCESS);
}

//マップというよりフィールドの初期化
void init_map(map_t* map_p)
{
  int i,j;

  for(i=0;i<map_p->height;i++){
    for(j=0;j<map_p->width;j++){
      map_p->field_p[i][j].state=HIDDEN;
      map_p->field_p[i][j].is_mine=FALSE;
      map_p->field_p[i][j].around_mines=0;      
    }
  }

}

//タイルをポインタで交換する関数
void swap_tile(tile_t* a,tile_t* b)
{
  tile_t tmp = *a;
  
  *a=*b;
  *b=tmp;
}	    
  
//第二のmain関数のような形になった。
//ゲームプレイ中の動作を総括している。
void play(map_t* map_p)
{
  int is_explode=FALSE;
  int is_cleared=FALSE;
  time_t start,end;

  //初回操作の処理。
  show_field(map_p);
  open_tile(map_p,TRUE);

  //計時を開始してゲームループに入る。
  time(&start);
  //ループの最初に表示関数、最後に操作関数を入れることでゲームオーバ時にもマップを表示する
  while(1){
    
    show_field(map_p);
    is_cleared=judge_cleared(map_p);

    //後の操作関数でとるゲームオーバ判定を表示後に行う
    if(is_cleared == TRUE || is_explode == TRUE){
      break;
    }

    //ゲームオーバ判定をとりながら操作
    is_explode=open_tile(map_p,FALSE);
  } 
  time(&end);
  
  game_over(is_cleared,difftime(end,start),map_p);
}

//マップを表示する
void show_field(map_t* map_p)
{
  int i,j;
  char c='.';

  //ループを負から始めるようにしてマス目表示を行った。
  for(i=-1;i<map_p->height;i++){
    for(j=-1;j<map_p->width;j++){
      if(i==-1){
	if(j==-1){
	  printf("   ");
	}else{
	  printf("%3d",j);
	}
      }else{
	if(j==-1){
	  printf("%3d",i);
	}else{
	  //平常時はマスの状態から表示するべき文字を決め、出力する
	  c=select_letter(map_p->field_p,j,i);
	  printf("%3c",c);
	}
      }
    }
    puts("");
  }
  printf("\nmines:%d\n",map_p->total_mine);
}

//マップ上の１マスに表示する文字を求める
char select_letter(tile_t** field_p,int x,int y)
{
  char c='\0';

  /*
    初期状態は'.'、旗は'F'、わからないところは'?'、開けて周りに地雷がないところはスペース、
    周りに地雷があるところはその個数、開けてしまった地雷は'!'
   */
  if(field_p[y][x].state==FLAGED){
    c='F';
  }else if(field_p[y][x].state==QUEST){
    c='?';
  }else if(field_p[y][x].state==HIDDEN){
    c='.';
  }else if(field_p[y][x].state==SHOWN){
    if(field_p[y][x].is_mine == FALSE){
      if(field_p[y][x].around_mines == 0){
	c=' ';
      }else{
	c=field_p[y][x].around_mines + '0';
      }
    }else{
      c='!';
    }
  }

  return c;
}

  
//タイルを開くマスター関数
//地雷を踏んだらTRUEを返す
int open_tile(map_t* map_p,int is_first)
{
  int x,y;
  int type;
  int is_explode=FALSE;

  //初回だけマップセット等の特殊な処理を行う
  if(is_first==TRUE){
    do{
      type=get_input(&x,&y,map_p);
    }while(type!=OPEN);
    set_mines(map_p,x,y);
    open_one(map_p,x,y);
  }else{

    //入力関数から操作タイプをとり、それに応じて関数を実行
    type=get_input(&x,&y,map_p);
    
    if(type==OPEN){
      is_explode=open_one(map_p,x,y);
    }else if(type==FLAG){
      put_flag(map_p,x,y);
    }else if(type==AROUND){
      is_explode=open_around(map_p,x,y);
    }else if(type==QUEST){
      put_quest(map_p,x,y);
    }
    
  }
  
  return is_explode;
}

//座標をとって操作のtypeを返す
int get_input(int* x,int* y,map_t* map_p)
{
  char buf[255]={};
  int tmp_x,tmp_y;
  char ident;
  int is_correct;

  // 最初のアルファベットで操作の種類を指定し、その後x,yで座標を指定する。
  // 単開け：o、旗立て：f、周囲開け:a、疑問視:q
  // ex.) o1 2
  printf("操作と座標を指定してください：");
  do{
    tmp_x=0;
    tmp_y=0;
    ident=0;
    
    fgets(buf,255,stdin);

    is_correct=get_parameter(buf,&ident,&tmp_x,&tmp_y,map_p);

    if(is_correct==FALSE){
      printf("不正な入力です。再入力してください：");
    }
  }while(is_correct==FALSE);

  *x=tmp_x;
  *y=tmp_y;

  return judge_ident(ident);

}

//入力で与えられたchar型の文字を開け方のコードに直して返す
int judge_ident(char ident)
{
  switch(ident){
  case 'o':
    return OPEN;
    break;
  case 'f':
    return FLAG;
    break;
  case 'a':
    return AROUND;
    break;
  case 'q':
    return QUEST;
    break;
  default:
    return -1;
    break;
  }

}

//入力に問題がなければ値を設定してTRUEを返す、問題があればFALSEを返す
int get_parameter(char buf[255],char* ident,int* x,int* y,map_t* map_p)
{
  int tmp_x,tmp_y;
  char tmp_ident;
  
  if(check_string(buf)==TRUE){
    //入力に問題がない場合まず変数にデータをとり、その後適用
    sscanf(buf,"%c%d %d",&tmp_ident,&tmp_x,&tmp_y);
    if(tmp_x<0 || tmp_x>=map_p->width || tmp_y<0 || tmp_y>=map_p->height){
      return FALSE;
    }

    *x=tmp_x;
    *y=tmp_y;

    *ident=tmp_ident;

    return TRUE;
  }else{
    return FALSE;
  }

}


// 入力を走査し、不正な入力にFALSEを、正しい入力にTRUEを返す(整数の大きさは問わない)
int check_string(char buf[255])
{
  int i,j;

  if(buf[0]!= 'o' && buf[0]!='f' && buf[0]!='a' && buf[0]!= 'q'){
    return FALSE;
  }
  j=1;
  for(i=0;i<2;i++){
    if(isdigit(buf[j])==0){
      return FALSE;
    }
    while(i==0 ? buf[j]!=' ' : buf[j]!='\n'){
      if(isdigit(buf[j])==0){
	return FALSE;
      }
      j++;
    }
    j++;
  }
  
  return TRUE;
}


//地雷を設置する関数
void set_mines(map_t* map_p,int exception_x,int exception_y)
{
  int i;
  int is_rejected=FALSE;
  
  // 個数分だけマップの左上から地雷を設置する。
  for(i=0;i<map_p->height*map_p->width;i++){
    if(i>=map_p->total_mine + is_rejected){
      break;
    }

    // 最初に開けた位置には地雷が置かれないようにする
    if(i/map_p->width==exception_y && i%map_p->width==exception_x){
      is_rejected=TRUE;
    }else{
      map_p->field_p[i/map_p->width][i%map_p->width].is_mine=TRUE;
    }
    
  }  

  // 設置した地雷をばらばらに配置しなおす
  shuffle_field(map_p,exception_x,exception_y);
}

// 初期配置の地雷をランダマイズする
void shuffle_field(map_t* map_p,int exception_x,int exception_y)
{
  int i;
  int h,w;
  int is_rejected=FALSE;


  for(i=0;i<map_p->total_mine+is_rejected;i++){
    // 初期の指定位置はシャッフルを避け、地雷を置かないようにする
    if(i/map_p->width==exception_y && i%map_p->width==exception_x){
      is_rejected=TRUE;
    }else{
      do{
	h=rand()%map_p->height;
	w=rand()%map_p->width;
      }while(h==exception_y && w==exception_x);
    swap_tile((map_p->field_p[i/map_p->width])+i%map_p->width,(map_p->field_p[h])+w);
    }
  }

}


//マスひとつだけを開く関数
int open_one(map_t* map_p,int x,int y)
{
  if(map_p->field_p[y][x].state!=SHOWN){
    chain(map_p,x,y);
  }
  
  return map_p->field_p[y][x].is_mine;
}

//旗を置く関数
void put_flag(map_t* map_p,int x,int y)
{
  if(map_p->field_p[y][x].state==FLAGED){
    map_p->field_p[y][x].state=HIDDEN;
  }else if(map_p->field_p[y][x].state!=SHOWN){
    map_p->field_p[y][x].state=FLAGED;
  }
}

//旗と数字が一致するマスの周りを開く関数
int open_around(map_t* map_p,int x,int y)
{
  int around_flags;
  int around_number;
  int is_explode=FALSE;
  int i,j;
  
  around_flags=count_around_flags(map_p,x,y);
  around_number=map_p->field_p[y][x].around_mines;
  // 旗の数が可視化された自分の数値と一致しており、数値があって周囲に旗があれば行う
  if(map_p->field_p[y][x].state==SHOWN){
    if(around_flags!=0 && around_number!=0){
      if(around_number==around_flags){
	//周囲のマップ外でない見えないマスのみ開く
	for(i=x-1;i<=x+1;i++){
	  for(j=y-1;j<=y+1;j++){
	    if(i>=0 && j>=0 && i<map_p->height && j<map_p->width){
	      if(map_p->field_p[j][i].state==HIDDEN){
		chain(map_p,i,j);
		if(map_p->field_p[j][i].is_mine==TRUE){
		  is_explode=TRUE;
		}
	      }
	    }
	  }
	}
      }
    }
  }

  return is_explode;
}

//地雷があるかわからないマスを指定する関数
void put_quest(map_t* map_p,int x,int y)
{
  if(map_p->field_p[y][x].state==QUEST){
    map_p->field_p[y][x].state=HIDDEN;
  }else if(map_p->field_p[y][x].state!=SHOWN){
    map_p->field_p[y][x].state=QUEST;
  }

}

//再帰によって周囲に地雷のないマスをまとめてひらく関数
void chain(map_t* map_p,int x,int y)
{
  int i,j;
  int count=0;
  
  map_p->field_p[y][x].state=SHOWN;
  map_p->field_p[y][x].around_mines=count_around_mines(map_p,x,y);

  // 探索の結果周囲に地雷がなければ、x,yの周囲からさらに探索を行う
  if(map_p->field_p[y][x].around_mines==0){
    for(i=y-1;i<=y+1;i++){
      for(j=x-1;j<=x+1;j++){
	if(i>=0 && j>=0 && i<map_p->height && j<map_p->width){
	  if(count==0 && map_p->field_p[i][j].state==HIDDEN){
	    if(i!=y || j!=x){
	      chain(map_p,j,i);
	    }
	  }
	}
      }
    }
  }
  
}

//周囲の地雷の数を数える関数
int count_around_mines(map_t* map_p,int x,int y)
{
  int i,j;
  int count;

  count=0;
  // x,yの周囲の地雷を探索、マップ外は除外
  for(i=y-1;i<=y+1;i++){
    for(j=x-1;j<=x+1;j++){
      if(i>=0 && j>=0 && i<map_p->height && j<map_p->width && map_p->field_p[i][j].is_mine==TRUE){
	count++;
      }
    }
  }

  return count;
}

//周囲の旗の数を数える関数
int count_around_flags(map_t* map_p,int x,int y)
{
  int i,j;
  int count;


  count=0;
  // x,yの周囲の旗を探索、マップ外は除外
  for(i=y-1;i<=y+1;i++){
    for(j=x-1;j<=x+1;j++){
      if(i>=0 && j>=0 && i<map_p->height && j<map_p->width && map_p->field_p[i][j].state==FLAGED){
	count++;
      }
    }
  }

  return count;
}
  

//クリアしたかどうかを判定する関数
int judge_cleared(map_t* map_p)
{
  int count=0;
  int i,j;
  
  //全ての地雷でないタイルを開けたらTRUEを返す
  for(i=0;i<map_p->height;i++){
    for(j=0;j<map_p->width;j++){
      if(map_p->field_p[i][j].is_mine==FALSE && map_p->field_p[i][j].state==SHOWN){
	count++;
      }
    }
  }
  
  if(count>=(map_p->width*map_p->height)-map_p->total_mine){
    return TRUE;
  }else{
    return FALSE;
  }

}

//ゲーム終了時にクリアしたかどうかに応じてメッセージを出したりファイル出力を行ったりする
void game_over(int is_cleared,int time,map_t* map_p)
{
  if(is_cleared==TRUE){
    printf("すべてのMを見切ることができました！クリア！\ntime:%d\n\n",time);
    create_result(time,map_p);
  }else{
    printf("あなたは見事にMを踏みぬいたスーパープレイヤーです！安らかに眠れ！\n");
  }

}

//クリア時に成績をファイルに出力するマスター関数
void create_result(int time,map_t* map_p)
{
  FILE* fp;
  int score;
  int is_empty=FALSE;
  int high_score;

  if((fp=fopen("result.dat","r")) == NULL){
    is_empty=TRUE;
  }else{
    high_score=get_high_score(fp);
  }
  
  fp=fopen("result.dat","a");
  
  if(fp==NULL){
    printf("\nfailed to save data.\n");
  }else{
    write_date(fp);
    score=write_score(fp,time,map_p);
    puts("あなたの栄光はデータファイルに刻まれました！");
    printf("あなたのM除去計画における活躍に%d点の功績が認められました。おめでとう！\n",score);
    if(is_empty==TRUE){
      printf("あなたが初のmsweeperです！！\n");
    }else{
      if(high_score<score){
	printf("あなたは歴代で最も偉大なmsweeperです！！(先人の記録：%d)\n",high_score);
      }else{
	printf("先人はあなたより優れたmsweeperでした。(先人の記録:%d)\n",high_score);
      }
    }
    fclose(fp);
  }
  
}

int get_high_score(FILE* fp)
{
  int high_score=-1;
  int score=-1;
  char buf[255];
  
  while(fgets(buf,255,fp)!=NULL){
    sscanf(buf,"score:%d",&score);
    if(high_score<score){
      high_score=score;
    }
  }

  return high_score;
}

//日付、時分秒をファイルに出力する関数
void write_date(FILE* fp)
{
  time_t t;
  struct tm* t_parameter;
  int year,month,day,hour,min,sec;
  
  time(&t);
  t_parameter=localtime(&t);

  year=t_parameter->tm_year+1900;
  month=t_parameter->tm_mon+1;
  day=t_parameter->tm_mday;
  hour=t_parameter->tm_hour;
  min=t_parameter->tm_min;
  sec=t_parameter->tm_sec;
  
  fprintf(fp,"%4d年%d月%d日 %02d:%02d:%02d\n",year,month,day,hour,min,sec);
}

//成績とマップ設定をファイルに出力する関数
int write_score(FILE* fp,int time,map_t* map_p)
{
  int score;
  const char* diff_name[]={"easy","normal","hard"};

  score=(int)(map_p->total_mine*3+((double)map_p->total_mine*0.08)*(map_p->width*map_p->height-time));
  
  fprintf(fp,"difficulty:%s\n",map_p->difficulty==0 ? "*original map" : diff_name[map_p->difficulty]);
  fprintf(fp,"map width:%d,map height:%d,number of mines:%d\n",map_p->width,map_p->height,map_p->total_mine);
  fprintf(fp,"score:%d\n\n",score);

  return score;
}


//動的に確保したフィールドのメモリを解放する関数
void free_field(map_t* map_p)
{
  int i;

  for(i=0;i<map_p->height;i++){
    free(map_p->field_p[i]);
  }

  free(map_p->field_p);
}

