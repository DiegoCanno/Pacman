/*
 * GAME SCENE
 * Copyright © 2018+ Ángel Rodríguez Ballesteros
 *
 * Distributed under the Boost Software License, version  1.0
 * See documents/LICENSE.TXT or www.boost.org/LICENSE_1_0.txt
 *
 * angel.rodriguez@esne.edu
 */

#include <basics/Director>
#include "Game_Scene.hpp"
#include "Sprite.hpp"
#include <basics/log> // basics::log.d("message");
#include "Menu_Scene.hpp"

#include <cstdlib>
#include <basics/Canvas>
#include <basics/Director>

using namespace basics;
using namespace std;


namespace example
{
    // ---------------------------------------------------------------------------------------------
    // ID y ruta de las texturas que se deben cargar para esta escena. La textura con el mensaje de
    // carga está la primera para poder dibujarla cuanto antes:

    Game_Scene::Texture_Data Game_Scene::textures_data[] =
    {
        { ID(loading),    "game-scene/loading.png"        },
        { ID(ball),       "game-scene/ball.png"           },
        { ID(up),       "button/arriba.png"               },
        { ID(down),       "button/abajo.png"              },
        { ID(left),       "button/izquierda.png"          },
        { ID(right),       "button/derecha.png"           },
        { ID(pacman),       "Character/pacman.png"        },
        { ID(phantom),       "Character/phantom1.png"     },
        { ID(phantomeat),       "Character/phantomeat.png"      },
        { ID(wall),                   "Character/wall.png"},
        { ID(coin),                 "Character/pelota.png"},
        { ID(special_coin),   "Character/special_coin.png"},

    };

    // Pâra determinar el número de items en el array textures_data, se divide el tamaño en bytes
    // del array completo entre el tamaño en bytes de un item:

    unsigned Game_Scene::textures_count = sizeof(textures_data) / sizeof(Texture_Data);

    // ---------------------------------------------------------------------------------------------
    // Definiciones de los atributos estáticos de la clase:

    constexpr float Game_Scene::  ball_speed;
    constexpr float Game_Scene::player_speed;

    // ---------------------------------------------------------------------------------------------

    Game_Scene::Game_Scene()
    {
        // Se establece la resolución virtual (independiente de la resolución virtual del dispositivo).
        // En este caso no se hace ajuste de aspect ratio, por lo que puede haber distorsión cuando
        // el aspect ratio real de la pantalla del dispositivo es distinto.

        canvas_width  = 1280;
        canvas_height =  720;

        // Se inicia la semilla del generador de números aleatorios:

        srand (unsigned(time(nullptr)));

        // Se inicializan otros atributos:

        initialize ();
    }

    // ---------------------------------------------------------------------------------------------
    // Algunos atributos se inicializan en este método en lugar de hacerlo en el constructor porque
    // este método puede ser llamado más veces para restablecer el estado de la escena y el constructor
    // solo se invoca una vez.

    bool Game_Scene::initialize ()
    {
        state     = LOADING;
        suspended = true;
        gameplay  = UNINITIALIZED;

        return true;
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::suspend ()
    {
        suspended = true;               // Se marca que la escena ha pasado a primer plano
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::resume ()
    {
        suspended = false;              // Se marca que la escena ha pasado a segundo plano
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::handle (Event & event)
    {
        if (state == RUNNING)               // Se descartan los eventos cuando la escena está LOADING
        {
            if (gameplay == WAITING_TO_START)
            {
                start_playing ();           // Se empieza a jugar cuando el usuario toca la pantalla por primera vez
            }
            else switch (event.id)
            {
                case ID(touch-started):// El usuario toca la pantalla
                {
                    final_x = *event[ID(x)].as< var::Float > ();
                    final_y = *event[ID(y)].as< var::Float > ();
                    final_point = {final_x,final_y};

                }
                case ID(touch-moved):
                {

                    break;
                }

                case ID(touch-ended):       // El usuario deja de tocar la pantalla
                {
                    final_x = *event[ID(x)].as< var::Float > ();
                    final_y = *event[ID(y)].as< var::Float > ();
                    final_point = {final_x,final_y};
                    if (up_button->contains(final_point))
                    {
                        pacman->set_speed_y(200);
                        pacman->set_speed_x(0);
                    }
                    else if (down_button->contains(final_point))
                    {
                        pacman->set_speed_y(-200);
                        pacman->set_speed_x(0);


                    }
                    else if (left_button->contains(final_point))
                    {
                        pacman->set_speed_x(-200);
                        pacman->set_speed_y(0);
                    }
                    else if (right_button->contains(final_point))
                    {
                        pacman->set_speed_x(200);
                        pacman->set_speed_y(0);
                    }

                    break;
                }
            }
        }
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::update (float time)
    {
        if (!suspended) switch (state)
        {
            case LOADING: load_textures  ();     break;
            case RUNNING: run_simulation (time); break;
            case ERROR:   break;


        }


    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::render (Context & context)
    {
        if (!suspended)
        {
            // El canvas se puede haber creado previamente, en cuyo caso solo hay que pedirlo:

            Canvas * canvas = context->get_renderer< Canvas > (ID(canvas));

            // Si no se ha creado previamente, hay que crearlo una vez:

            if (!canvas)
            {
                 canvas = Canvas::create (ID(canvas), context, {{ canvas_width, canvas_height }});
            }

            // Si el canvas se ha podido obtener o crear, se puede dibujar con él:

            if (canvas)
            {
                canvas->clear ();

                switch (state)
                {
                    case LOADING: render_loading   (*canvas); break;
                    case RUNNING: render_playfield (*canvas); break;
                    case ERROR:   break;
                }
            }


        }
    }

    // ---------------------------------------------------------------------------------------------
    // En este método solo se carga una textura por fotograma para poder pausar la carga si el
    // juego pasa a segundo plano inesperadamente. Otro aspecto interesante es que la carga no
    // comienza hasta que la escena se inicia para así tener la posibilidad de mostrar al usuario
    // que la carga está en curso en lugar de tener una pantalla en negro que no responde durante
    // un tiempo.

    void Game_Scene::load_textures ()
    {
        if (textures.size () < textures_count)          // Si quedan texturas por cargar...
        {
            // Las texturas se cargan y se suben al contexto gráfico, por lo que es necesario disponer
            // de uno:

            Graphics_Context::Accessor context = director.lock_graphics_context ();

            if (context)
            {
                // Se carga la siguiente textura (textures.size() indica cuántas llevamos cargadas):

                Texture_Data   & texture_data = textures_data[textures.size ()];
                Texture_Handle & texture      = textures[texture_data.id] = Texture_2D::create (texture_data.id, context, texture_data.path);

                // Se comprueba si la textura se ha podido cargar correctamente:

                if (texture) context->add (texture); else state = ERROR;

                // Cuando se han terminado de cargar todas las texturas se pueden crear los sprites que
                // las usarán e iniciar el juego:
            }
        }
        else
        if (timer.get_elapsed_seconds () > 1.f)         // Si las texturas se han cargado muy rápido
        {                                               // se espera un segundo desde el inicio de
            create_sprites ();                          // la carga antes de pasar al juego para que
            restart_game   ();                          // el mensaje de carga no aparezca y desaparezca
                                                        // demasiado rápido.
            state = RUNNING;
        }
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::create_sprites ()
    {
        create_map ();


        Graphics_Context::Accessor context = director.lock_graphics_context ();

        // Se crean y configuran los sprites del fondo:

        //Se crean los botones
        Sprite_Handle  up_button_sprite(new Sprite( textures[ID(up)].get () ));
        Sprite_Handle  down_button_sprite(new Sprite( textures[ID(down)].get () ));
        Sprite_Handle  left_button_sprite(new Sprite( textures[ID(left)].get () ));
        Sprite_Handle  right_button_sprite(new Sprite( textures[ID(right)].get () ));

        //se crean los sprites de los elementos del juego pacman, etc
        Sprite_Handle pacman_sprite(new Sprite(textures[ID(pacman)].get()));

        Sprite_Handle phantom_sprite(new Sprite(textures[ID(phantom)].get()));

        Sprite_Handle wall_sprite(new Sprite(textures[ID(wall)].get()));

        Sprite_Handle coin_sprite(new Sprite(textures[ID(coin)].get()));

        Sprite_Handle special_coin_sprite(new Sprite(textures[ID(special_coin)].get()));

        Sprite_Handle phantom_eat_sprite(new Sprite(textures[ID(phantom_eat)].get()));


        sprites.push_back (up_button_sprite);
        sprites.push_back (down_button_sprite);
        sprites.push_back (left_button_sprite);
        sprites.push_back (right_button_sprite);

        sprites.push_back(pacman_sprite);
        pacman_sprite->set_scale(0.1);

        sprites.push_back(phantom_sprite);
        phantom_sprite->set_scale(0.1);

        sprites.push_back(wall_sprite);
        wall_sprite->set_scale(0.05);

        sprites.push_back(coin_sprite);
        coin_sprite->set_scale(0.5);

        sprites.push_back(special_coin_sprite);
        special_coin_sprite->set_scale(0.05);

        sprites.push_back(phantom_eat_sprite);
        phantom_eat_sprite->set_scale(0.1);



        up_button_sprite->set_scale(0.2);
        down_button_sprite->set_scale(0.2);
        left_button_sprite->set_scale(0.2);
        right_button_sprite->set_scale(0.2);

        coin_sprite->set_scale(0.1);


        // Se guardan punteros a los sprites que se van a usar frecuentemente:
        pacman        = pacman_sprite.get();
        phantom       = phantom_sprite.get();
        wall          = wall_sprite.get();
        coin          = coin_sprite.get();
        special_coin  = special_coin_sprite.get();
        phantom_eat   = phantom_eat_sprite.get();

        //prueba
        up_button = up_button_sprite.get();
        down_button = down_button_sprite.get();
        left_button = left_button_sprite.get();
        right_button = right_button_sprite.get();
    }

    // ---------------------------------------------------------------------------------------------


    void Game_Scene::restart_game()
    {
        //Posicion inicical al empezar el juego de los elementos del mismo
        up_button->set_position({canvas_width/2,canvas_height/3});
        down_button->set_position({canvas_width/2,canvas_height/6});
        left_button->set_position({(canvas_width/2)-100,canvas_height/6});
        right_button->set_position({(canvas_width/2)+100,canvas_height/6});


        //posicion del bueno de pacman
        pacman->set_position({(canvas_width/2),(canvas_height/2)-100});

        //posicion phantom
        phantom->set_position({(canvas_width/2)+40,canvas_height/2});

        //posicion del wall del que en el futuro haremos mas cositas
        //wall->set_position({(canvas_width/2)+300,canvas_height/2});

        //posicion de la coin del que en el futuro haremos mas cositas
        //coin->set_position({(canvas_width/2)-300,canvas_height/2});

        special_coin->set_position({(canvas_width/2),canvas_height/4});





        gameplay = WAITING_TO_START;
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::start_playing ()
    {
        // Se genera un vector de dirección al azar:

        Vector2f random_direction
        (
            float(rand () % int(canvas_width ) - int(canvas_width  / 2)),
            float(rand () % int(canvas_height) - int(canvas_height / 2))
        );

        // Se hace unitario el vector y se multiplica un el valor de velocidad para que el vector
        // resultante tenga exactamente esa longitud:

        // se le da una velocidad inicial al fantasma a partir de ahi t
        phantom->set_speed_x(200);
        phantom_eat->hide();

        gameplay = PLAYING;
    }

    // ---------------------------------------------------------------------------------------------

    void Game_Scene::run_simulation (float time)
    {
        // Se actualiza el estado de todos los sprites:

        for (auto & sprite : sprites)
        {
            sprite->update (time);
        }


        phantom_ia();
        check_collision();
        special_coin_event();


    }

    // ---------------------------------------------------------------------------------------------



    void Game_Scene::phantom_ia (){
        for(int i=0;i<266;++i){
            if(phantom->intersects(*walls[i])) {
                if (phantom->intersects(*walls[i]) && phantom->get_speed_y() != 0) {
                    if (phantom->get_speed_y() < 0) {
                        phantom->set_position_y(phantom->get_position_y() + 5.f);
                        phantom->set_speed_y(0);
                        RandomNumber = random() % 3 + 1;

                        if (RandomNumber == 1) {
                            phantom->set_speed_x(200);
                        } else if (RandomNumber == 2) {
                            phantom->set_speed_y(200);
                        } else if (RandomNumber == 3) {
                            phantom->set_speed_x(-200);
                        }
                    }
                    if (phantom->get_speed_y() > 0) {
                        phantom->set_position_y(phantom->get_position_y() - 5.f);
                        RandomNumber = random() % 3 + 1;
                        phantom->set_speed_y(0);


                        if (RandomNumber == 1) {
                            phantom->set_speed_x(200);


                        } else if (RandomNumber == 2) {
                            phantom->set_speed_y(-200);
                        } else if (RandomNumber == 3) {
                            phantom->set_speed_x(-200);
                        }
                    }
                } else if (phantom->intersects(*walls[i]) && phantom->get_speed_x() != 0) {

                    if (phantom->get_speed_x() < 0) {
                        phantom->set_position_x(phantom->get_position_x() + 5.f);
                        RandomNumber = random() % 3 + 1;
                        phantom->set_speed_x(0);
                        if (RandomNumber == 1) {
                            phantom->set_speed_x(200);
                        } else if (RandomNumber == 2) {
                            phantom->set_speed_y(-200);
                        } else if (RandomNumber == 3) {
                            phantom->set_speed_y(200);
                        }
                    } else if (phantom->get_speed_x() > 0) {

                        phantom->set_position_x(phantom->get_position_x() - 5.f);
                        RandomNumber = random() % 3 + 1;
                        phantom->set_speed_x(0);

                        if (RandomNumber == 1) {
                            phantom->set_speed_x(-200);
                        } else if (RandomNumber == 2) {
                            phantom->set_speed_y(-200);
                        } else if (RandomNumber == 3) {
                            phantom->set_speed_y(200);
                        }
                    }
                }

            }
        }


    }

    // maneja el que el fantasma  que se crea enseñe un sprite encima de el
    //si se come al fantasma este vuelve a su posicion inicical y vuele a iniciar el movimento
    void Game_Scene::special_coin_event(){

        if(pacman->intersects(*special_coin)){
            timer.reset();
            puedecomer = true;
            phantom_eat->is_visible();
        }
        if(pacman->intersects(*phantom) && timer.get_elapsed_seconds () < 10.f && puedecomer) {

            phantom_eat->hide();
            phantom->set_position({(canvas_width/2)+40,canvas_height/2});
            phantom->set_speed_y(200);
        }
        else if (timer.get_elapsed_seconds () > 10.f){
            phantom_eat->hide();
            puedecomer=false;
        }
    }

    // ---------------------------------------------------------------------------------------------





    // ---------------------------------------------------------------------------------------------
    //estas son las colisiones que tiene pacman tanto con el fantasma como con las monedas como con las paredes, no he encontrado la forma mas optima de hacerlo
    //ya que esta recorriendo tres veces el array de 266
    void Game_Scene::check_collision() {

        for(int i=0;i<266;++i){
            if(pacman->intersects(*walls[i]) && pacman->get_speed_y() !=0 )
            {
                if(pacman->get_speed_y()<0)
                {
                    pacman->set_position_y(pacman->get_position_y()+5.f);
                    pacman->set_speed_y(0);
                }
                if( pacman->get_speed_y()>0)
                {
                    pacman->set_position_y(pacman->get_position_y()-5.f);
                    pacman->set_speed_y(0);
                }
            }
            else if(pacman->intersects(*walls[i]) && pacman->get_speed_x()!=0){
                if(pacman->get_speed_x()<0){
                    pacman->set_position_x(pacman->get_position_x()+5.f);
                    pacman->set_speed_x(0);
                }
                else if(pacman->get_speed_x()>0){
                    pacman->set_position_x(pacman->get_position_x()-5.f);
                    pacman->set_speed_x(0);
                }
            }

        }
        for(int i=0;i<266;++i){
            if(pacman->intersects(*coins_a[i])){

                contadorMonedas++;
                coins_a[i]->set_position_y(4000);
                if(contadorMonedas == 4){

                    director.run_scene (shared_ptr< Scene >(new Menu_Scene));

                }
            }

        }
        if(pacman->intersects(*phantom)){
            director.run_scene (shared_ptr< Scene >(new Menu_Scene));
        }


    }


    // ---------------------------------------------------------------------------------------------

    void Game_Scene::render_loading (Canvas & canvas)
    {
        Texture_2D * loading_texture = textures[ID(loading)].get ();

        if (loading_texture)
        {
            canvas.fill_rectangle
            (
                { canvas_width * .5f, canvas_height * .5f },
                { loading_texture->get_width (), loading_texture->get_height () },
                  loading_texture
            );
        }
    }

    // ---------------------------------------------------------------------------------------------
    // Simplemente se dibujan todos los sprites que conforman la escena.

    void Game_Scene::render_playfield (Canvas & canvas)
    {
        for (auto & sprite : sprites)
        {
            sprite->render (canvas);

        }

        phantom_eat->set_position(phantom->get_position());
    }


    void Game_Scene::create_map ()
    {
        int f = 0;      // #fila
        int c = 0;      // #columna
        int i = 0;      // indice
        int xLoc, yLoc; // Posiciones en X e Y
        int x, y;       // Valores absolutos en X e Y
        Id id = ID(wall);
        Id idcoin = ID(coin);

        for (; f <= 20; ++f, ++i) // Recorre la matriz fila a fila de izquierda a derecha
        {

            xLoc = f + 1; // Ajustar las posiciones para multiplicar con ellas
            yLoc = c+ 1; //
            x = posXTablero + xLoc * escalar;
            y = posYTablero + yLoc * escalar;


            // Genera la carta boca abajo y guarda una referencia a esta
            if(mapa[i] ==1){
                Sprite_Handle casillaMap(new Sprite(textures[id].get()));

                casillaMap->set_position({x, y});
                casillaMap->set_scale(0.05);
                casillasSpr[i] = casillaMap.get();
                sprites.push_back(casillaMap);
            }
            if (mapa[i]==2){

                Sprite_Handle coin_sprite_v(new Sprite(textures[idcoin].get()));
                coin_sprite_v->set_position({x, y});
                coin_sprite_v->set_scale(0.01);
                casillasCoins[i] = coin_sprite_v.get();
                sprites.push_back(coin_sprite_v);


            }


            if (f == 20 && c < 12) // Salta a la siguiente fila siempre que haya una
            {
                f = -1;
                ++c;
            }
        }
    }





}
