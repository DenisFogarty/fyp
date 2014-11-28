#include <iostream>
#include <stdio.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

class DATA {

	public:
		ALLEGRO_MUTEX *mutex;
		ALLEGRO_COND *cond;
		float POSI_X;
		float POSI_Y;
		bool READY;
		bool RIGHT;
		bool UP;
		bool MODI_X;

		float MOUSE_X;
		float MOUSE_Y;

	DATA():
		mutex(al_create_mutex()),
		cond(al_create_cond()),
		READY (false),
		RIGHT (false),
		UP (false),
		MODI_X (true)
	{}

	~DATA(){
		al_destroy_mutex(mutex);
		al_destroy_cond(cond);
	}
};

class SHOOT_DATA {
	public:
		ALLEGRO_MUTEX * mutex;
		ALLEGRO_COND *cond;
		ALLEGRO_DISPLAY *display;
		float PLAY_X;
		float PLAY_Y;
		float CUR_X;
		float CUR_Y;
		bool READY;
		bool DRAW_LINE;
		float x1, y1, x2, y2;

	SHOOT_DATA():
		mutex(al_create_mutex()),
		cond(al_create_cond()),
		READY (false),
		DRAW_LINE (false)
	{}

	~SHOOT_DATA() {
		al_destroy_mutex(mutex);
		al_destroy_cond(cond);
	}
};

const float FPS = 60;
float INC_X = 0;
float INC_Y = 0;
const int SCREEN_W = 600;
const int SCREEN_H = 480;
const int PLAYER_SIZE = 32;

const int CURSON_SIZE = 20;


static void *func_thread(ALLEGRO_THREAD *thr, void *arg);

static void *func_shoot(ALLEGRO_THREAD *thr, void *arg);

int main() {
	ALLEGRO_EVENT_QUEUE *event_queue = NULL;
	ALLEGRO_TIMER       *timer		 = NULL;
	ALLEGRO_BITMAP      *player		 = NULL;
	ALLEGRO_BITMAP		*cursor		 = NULL;
	ALLEGRO_THREAD      *thread_1    = NULL;
	ALLEGRO_THREAD		*thread_2	 = NULL;
	ALLEGRO_THREAD		*thread_3	 = NULL;

	SHOOT_DATA sh_data;

	bool redraw = true;

	bool right = false;
	bool up = false;

	if(!al_init()) {
		fprintf(stderr, "Failed to initialize Allegro");
		return -1;
	}

	if(!al_install_keyboard()) {
		fprintf(stderr, "Failed to install keyboard");
		return -1;
	}

	timer = al_create_timer(1.0/FPS);
	if(!timer) {
		fprintf(stderr, "Failed to initialize timer");
		return -1;
	}

	sh_data.display = al_create_display(SCREEN_W, SCREEN_H);
	if(!sh_data.display) {
		fprintf(stderr, "Failed to initialize display");
		al_destroy_timer(timer);
		return -1;
	}

	player = al_create_bitmap(PLAYER_SIZE, PLAYER_SIZE);
	if(!player) {
		fprintf(stderr, "Failed to initialize player");
		al_destroy_timer(timer);
		al_destroy_display(sh_data.display);
		return -1;
	}

	cursor = al_create_bitmap(CURSON_SIZE, CURSON_SIZE);
	if(!cursor) {
		fprintf(stderr, "Failed to initialize cursor");
		al_destroy_timer(timer);
		al_destroy_display(sh_data.display);
		al_destroy_bitmap(player);
		return -1;
	}

	event_queue = al_create_event_queue();

	if(!event_queue) {
	   fprintf(stderr, "failed to create event_queue!\n");
	   al_destroy_bitmap(player);
	   al_destroy_bitmap(cursor);
	   al_destroy_display(sh_data.display);
	   al_destroy_timer(timer);
	   return -1;
	}

	if(!al_install_mouse()) {
		fprintf(stderr, "Failed to initialize mouse\n");
		al_destroy_bitmap(player);
		al_destroy_bitmap(cursor);
		al_destroy_display(sh_data.display);
		al_destroy_timer(timer);
		al_destroy_event_queue(event_queue);
		return -1;
	}

	al_register_event_source(event_queue, al_get_display_event_source(sh_data.display));
	al_register_event_source(event_queue, al_get_timer_event_source(timer));
	al_register_event_source(event_queue, al_get_keyboard_event_source());
	al_register_event_source(event_queue, al_get_mouse_event_source());

	al_set_target_bitmap(player);
	al_clear_to_color(al_map_rgb(255, 0, 255));

	al_set_target_bitmap(cursor);
	al_clear_to_color(al_map_rgb(255, 255, 0));

	al_set_target_bitmap(al_get_backbuffer(sh_data.display));

	//Creates instance of classes
	DATA data;

	//Creating thread for left/right movement
	thread_1 = al_create_thread(func_thread, &data);
	al_start_thread(thread_1);

	al_lock_mutex(data.mutex);
	while (!data.READY) {

		al_wait_cond(data.cond, data.mutex);

	}
	al_unlock_mutex(data.mutex);

	//Creating thread for up/down movement
	data.MODI_X = false;
	thread_2 = al_create_thread(func_thread, &data);
	al_start_thread(thread_2);

	al_lock_mutex(data.mutex);
	while (!data.READY) {

	   al_wait_cond(data.cond, data.mutex);

	}
	al_unlock_mutex(data.mutex);

	//Starting timer
	al_start_timer(timer);

	//Setting initial position of player
	data.POSI_X = 0;
	data.POSI_Y = 0;


	while(1)
	{
		ALLEGRO_EVENT ev;
		al_wait_for_event(event_queue, &ev);

		if(ev.type == ALLEGRO_EVENT_TIMER) {
			redraw = true;
		}
		else if(ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
			break;
		}

		//Moves character left/right
		else if(ev.type == ALLEGRO_EVENT_KEY_DOWN && (ev.keyboard.keycode == ALLEGRO_KEY_D || ev.keyboard.keycode == ALLEGRO_KEY_A)) {
			if(ev.keyboard.keycode == ALLEGRO_KEY_D) {
				data.RIGHT = true;
				INC_X = 0.1;
				right = true;
			}
			else if(ev.keyboard.keycode == ALLEGRO_KEY_A) {
				data.RIGHT = false;
				INC_X = -0.1;
				right = false;
			}

		}

		//Moves character up/down
		else if(ev.type == ALLEGRO_EVENT_KEY_DOWN && (ev.keyboard.keycode == ALLEGRO_KEY_W || ev.keyboard.keycode == ALLEGRO_KEY_S)) {
			if(ev.keyboard.keycode == ALLEGRO_KEY_S) {
				data.UP = false;
				INC_Y = 0.1;
				up = false;
			}
			else if(ev.keyboard.keycode == ALLEGRO_KEY_W) {
				data.UP = true;
				INC_Y = -0.1;
				up = true;
            }
		}

		//Stops left/right movement when key released
		else if(ev.type == ALLEGRO_EVENT_KEY_UP && (ev.keyboard.keycode == ALLEGRO_KEY_D || ev.keyboard.keycode == ALLEGRO_KEY_A)) {
			if(right && ev.keyboard.keycode == ALLEGRO_KEY_D) {
				INC_X = 0;
			}
			else if(!right && ev.keyboard.keycode == ALLEGRO_KEY_A) {
				INC_X = 0;
            }
		}

		//Stops up/down movement when key released
		else if(ev.type == ALLEGRO_EVENT_KEY_UP && (ev.keyboard.keycode == ALLEGRO_KEY_W || ev.keyboard.keycode == ALLEGRO_KEY_S)) {
			if(!up && ev.keyboard.keycode == ALLEGRO_KEY_S) {
				INC_Y = 0;
			}
			else if(up && ev.keyboard.keycode == ALLEGRO_KEY_W) {
				INC_Y = 0;
			}
		}

		else if(ev.type == ALLEGRO_EVENT_MOUSE_AXES || ev.type == ALLEGRO_EVENT_MOUSE_ENTER_DISPLAY) {
			data.MOUSE_X = ev.mouse.x;
			data.MOUSE_Y = ev.mouse.y;
		}

		else if(ev.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN) {
			al_lock_mutex(sh_data.mutex);
			al_lock_mutex(data.mutex);

			sh_data.PLAY_X = data.POSI_X;
			sh_data.PLAY_Y = data.POSI_Y;

			sh_data.CUR_X = data.MOUSE_X;
			sh_data.CUR_Y = data.MOUSE_Y;

			al_unlock_mutex(sh_data.mutex);
			al_unlock_mutex(data.mutex);

			thread_3 = al_create_thread(func_shoot, &sh_data);
			al_start_thread(thread_3);
		}

		if(redraw && al_is_event_queue_empty(event_queue)) {
			redraw = false;

			al_lock_mutex(data.mutex);
			float X = data.POSI_X;
			float Y = data.POSI_Y;
			al_unlock_mutex(data.mutex);

			al_clear_to_color(al_map_rgb(0, 0, 0));

			al_draw_bitmap(player, X, Y, 0);

			al_draw_bitmap(cursor, data.MOUSE_X, data.MOUSE_Y, 0);

			if(sh_data.DRAW_LINE) {
				al_draw_line(sh_data.x1, sh_data.y1, sh_data.x2, sh_data.y2, ALLEGRO_COLOR(al_map_rgb(255, 0, 0)), 4);
                std::cout << "x1y1" << sh_data.x1 << " " << sh_data.y1 << std::endl;
                std::cout << "x2y2" << sh_data.x2 << " " << sh_data.y2 << std::endl;
			}

			al_flip_display();
		}
	}


	al_destroy_thread(thread_1);
	al_destroy_thread(thread_2);
	al_destroy_bitmap(player);
	al_destroy_bitmap(cursor);
	al_destroy_timer(timer);
	al_destroy_display(sh_data.display);
	al_destroy_event_queue(event_queue);

	return 1;
}

static void *func_thread(ALLEGRO_THREAD *thr, void *arg) {
	DATA *data  = (DATA*) arg;

	al_lock_mutex(data->mutex);

	bool modi_x = data->MODI_X;

	data->READY = true;

	al_broadcast_cond(data->cond);

	al_unlock_mutex(data->mutex);

	while(!al_get_thread_should_stop(thr)) {

		if(data->RIGHT && data->POSI_X >= SCREEN_W - PLAYER_SIZE) {
			INC_X = 0;
		}
		else if(!data->RIGHT && data->POSI_X <= 0) {
			INC_X = 0;
		}

		if(data->UP && data->POSI_Y <= 0) {
			INC_Y = 0;
		}
		else if(!data->UP && data->POSI_Y >= SCREEN_H - PLAYER_SIZE) {
			INC_Y = 0;
		}

		if(modi_x) {
			al_lock_mutex(data->mutex);
			data->POSI_X += INC_X;
			al_unlock_mutex(data->mutex);
		}
		else {
			al_lock_mutex(data->mutex);
			data->POSI_Y += INC_Y;
			al_unlock_mutex(data->mutex);
		}

        al_rest(0.0002);
	}

	return NULL;
}

static void *func_shoot(ALLEGRO_THREAD *thr, void *arg) {
	SHOOT_DATA *sh_data = (SHOOT_DATA*) arg;

	al_lock_mutex(sh_data->mutex);

	sh_data->x1 = sh_data->PLAY_X, sh_data->y1 = sh_data->PLAY_Y;
	sh_data->x2 = sh_data->CUR_X / 100, sh_data->y2 = sh_data->CUR_Y / 100;
    std::cout << sh_data->x1 << " " << sh_data->y1 << std::endl;
    std::cout << sh_data->x2 << " " << sh_data->y2 << std::endl;

	float inc_x = sh_data->x2, inc_y = sh_data->y2;

	sh_data->READY = true;

	al_broadcast_cond(sh_data->cond);

	al_unlock_mutex(sh_data->mutex);


	while(!al_get_thread_should_stop(thr)) {
        while(sh_data->x2 < 600 && sh_data->y2 < 480) {
			al_lock_mutex(sh_data->mutex);
			sh_data->x1 += inc_x;
            sh_data->x2 =  sh_data->x1 + inc_x;
			sh_data->y1 += inc_y;
            sh_data->y2 =  sh_data->y1 + inc_y;
			sh_data->DRAW_LINE = true;
			al_unlock_mutex(sh_data->mutex);
			al_rest(0.01);

            //std::cout << sh_data->x1 << "\t" << sh_data->y1 << "\t\t" << sh_data->x2 << "\t" << sh_data->y2 << std::endl;
		}
		sh_data->DRAW_LINE = false;
		break;
	}
	return NULL;
}
