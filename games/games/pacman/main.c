#include <defs.h>
#include <proto.h>


int main(int argc, char** argv) {
	UNUSED(argc);
	UNUSED(argv);

	/* Initialize curses */
	initscr();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	curs_set(FALSE);
	timeout(0);

	enum game_mode game_mode = GAME_OVER;
	uint32 transition_timer = 0;
	bool32 running = TRUE;

	uint32 last_update = time_in_us();
	uint32 frame_timer = 0;
	uint32 high_score = 0;

	bool32 has_color_terminal = has_colors() && can_change_color();
	start_color();
	if (!has_color_terminal || COLORS != 256) {
		mvprintw(0, 0, "Warning: Colors not enabled for this terminal.\nAuthors suggestion: try running in the unicode-rxvt terminal.\nPress any key to continue.");
		mvprintw(0, 0, "%u colors available in this terminal. This game currently supports 256.", COLORS);
		timeout(-1);
		getch();
		timeout(0);
	} else {
		init_pair(BG_PAIR, COLOR_WHITE, COLOR_DARK_BACKGROUND);

		int bkgd_color = COLOR_GREY;

		init_pair(EMPTY_PAIR, COLOR_WHITE, bkgd_color);
		init_pair(WALL_PAIR, COLOR_WALL_BLUE, COLOR_DARK_BACKGROUND);
		init_pair(DOOR_PAIR, COLOR_DOOR_RED, COLOR_DARK_BACKGROUND);
		init_pair(PACMAN_PAIR, COLOR_PACMAN_YELLOW, bkgd_color);
		init_pair(DOT_PAIR, COLOR_WHITE, bkgd_color);

		init_pair(GAME_OVER_TEXT_PAIR, COLOR_DOOR_RED, bkgd_color);

		init_pair(BLINKY_PAIR, COLOR_WHITE, COLOR_DOOR_RED);
		init_pair(INKY_PAIR, COLOR_WHITE, COLOR_LIGHT_BLUE);
		init_pair(PINKY_PAIR, COLOR_WHITE, COLOR_PINK);
		init_pair(CLYDE_PAIR, COLOR_WHITE, COLOR_ORANGE);
		init_pair(FRIGHT_PAIR, COLOR_WHITE, COLOR_WALL_BLUE);
		init_pair(FRIGHT_FLASH_PAIR, COLOR_WALL_BLUE, COLOR_WHITE);
		init_pair(EYES_PAIR, COLOR_WHITE, bkgd_color);

		init_pair(CHERRIES_PAIR, COLOR_DARK_RED, bkgd_color);
		init_pair(STRAWBERRY_PAIR, COLOR_BRIGHT_RED, bkgd_color);
		init_pair(PEACH_PAIR, COLOR_ORANGE, bkgd_color);
		init_pair(APPLE_PAIR, COLOR_DOOR_RED, bkgd_color);
		init_pair(GRAPES_PAIR, COLOR_PURPLE, bkgd_color);
		init_pair(GALAXIAN_PAIR, COLOR_ORANGE, bkgd_color);
		init_pair(BELL_PAIR, COLOR_PACMAN_YELLOW, bkgd_color);
		init_pair(KEY_PAIR, COLOR_LIGHT_BLUE, bkgd_color);

		bkgd(COLOR_PAIR(BG_PAIR));
	}

	bool32 debug_ghost_mode_overridden = FALSE, debug_no_death_mode = FALSE;
	struct view view = { 0 };

	struct ghost ghosts[NUM_GHOSTS];
	struct level_data level_data;
	struct level_constants level_constants;

	struct game_data game_data;

	start_new_game(&game_data, ghosts, &level_data, &level_constants);

	struct v2 pacman_turn_tile;

	uint32 frame_times[] = { 8, 16, 50, 100, 150 };
	int frame_time_index = 1;
	while (running) {
		/* Get Input */
		int ch;
		while ((ch = getch()) != -1) {
			switch (ch) {
				case 'q':
				case 'Q':
					running = FALSE;
					continue;
#ifdef DEBUG
				case 'j':
				case 'J':
					--frame_time_index;
					if (frame_time_index < 0) {
						frame_time_index = ARRAY_LENGTH(frame_times) - 1;
					}
					break;
				case 'k':
				case 'K':
					++frame_time_index;
					if (frame_time_index == ARRAY_LENGTH(frame_times)) {
						frame_time_index = 0;
					}
					break;
				case 'z':
				case 'Z':
					view.zoom_view = !view.zoom_view;
					break;
				case 'e':
				case 'E': {
					int i;
					for (i = 0; i < NUM_GHOSTS; ++i) {
						if (ghosts[i].ghost_house_state == OUTSIDE_GHOST_HOUSE) {
							ghosts[i].mode = EYES;
						}
					}
				} break;
				case 'f':
				case 'F':
					game_mode = LEVEL_TRANSITION;
					transition_timer = 0;
					break;
				case 'l':
				case 'L':
					game_mode = LOSING_A_LIFE;
					transition_timer = 0;
					break;
				case '+':
					add_extra_life(&game_data, &level_data);
					break;
				case '-':
					if (game_data.num_extra_lives > 0) {
						--game_data.num_extra_lives;
					}
					break;
				case 'c':
				case 'C':
					debug_ghost_mode_overridden = TRUE;
					level_data.is_chasing = !level_data.is_chasing;
					break;
				case 'd':
				case 'D':
					debug_no_death_mode = !debug_no_death_mode;
					break;
				case 'r':
				case 'R':
					reverse_ghosts(ghosts);
					break;
				case 'p':
				case 'P':
					start_new_game(&game_data, ghosts, &level_data, &level_constants);
					transition_timer = 0;
					game_mode = PAUSED_BEFORE_PLAYING;
					break;
#endif
			}
			if (game_mode == PLAYING && !level_data.pacman_turning) {
				switch (ch) {
					case KEY_LEFT:
						level_data.next_dir = LEFT;
						break;
					case KEY_RIGHT:
						level_data.next_dir = RIGHT;
						break;
					case KEY_UP:
						level_data.next_dir = UP;
						break;
					case KEY_DOWN:
						level_data.next_dir = DOWN;
						break;
				}
			} else if (game_mode == GAME_OVER) {
				start_new_game(&game_data, ghosts, &level_data, &level_constants);
				transition_timer = 0;
				game_mode = PAUSED_BEFORE_PLAYING;
			}
		}

		frame_timer += time_in_us() - last_update;
		last_update = time_in_us();

		if (frame_timer > MS(frame_times[frame_time_index])) {
			if (!level_data.ghost_eat_timer) {
				++level_data.pacman_chomp_timer;
				if (level_data.fruit_is_visible) {
					++level_data.fruit_timer;
					if (level_data.fruit_timer > seconds_to_frames(9)) {
						level_data.fruit_is_visible = FALSE;
					}
				}
			}

			switch (game_mode) {
				case PAUSED_BEFORE_PLAYING: {
					transition_timer++;
					if (transition_timer >= seconds_to_frames(2)) {
						transition_timer = 0;
						game_mode = PLAYING;
					}
				} break;
				case PLAYING: {
					uint32 pacman_speed;
					uint32 old_score = game_data.score;
					num_blocked_tiles = 0;
					if (level_data.ghost_eat_timer) {
						--level_data.ghost_eat_timer;
						if (!level_data.ghost_eat_timer) {
							level_data.ghost_to_be_eaten->mode = EYES;
							level_data.ghost_to_be_eaten = NULL;
						}
					} else if (level_data.pacman_powerup_timer) {
						--level_data.pacman_powerup_timer;
						pacman_speed = level_constants.pacman_powerup_speed;
						if (!level_data.pacman_powerup_timer) {
							int i;
							for (i = 0; i < NUM_GHOSTS; ++i) {
								if (ghosts[i].mode != EYES) {
									ghosts[i].mode = NORMAL;
								}
							}
						} else if (level_data.pacman_powerup_timer <= level_constants.num_ghost_flashes * 2 * GHOST_FLASH_TIME) {
							++level_data.ghost_flash_timer;
						}
					} else {
						pacman_speed = level_constants.pacman_speed;
					}
					if (level_data.pacman_eat_timer) {
						--level_data.pacman_eat_timer;
					} else if (!level_data.ghost_eat_timer) {
						if (level_data.pacman_dir != level_data.next_dir) {
							if (is_h_dir(level_data.pacman_dir) == is_h_dir(level_data.next_dir)) {
								level_data.pacman_dir = level_data.next_dir;
							} else {
								struct v2 blocked_pos_s = level_data.pacman_pos;
								bool32 blocked = test_pacman_dir_blocked(level_data.next_dir, &blocked_pos_s);
								if (blocked) {
									blocked_tiles[num_blocked_tiles++] = pos_to_tile(&blocked_pos_s);
								} else {
									level_data.pacman_turning = TRUE;
									pacman_turn_tile = pos_to_tile(&level_data.pacman_pos);
								}
							}
						}

						if (level_data.pacman_turning) {
							move_in_dir(level_data.next_dir, &level_data.pacman_pos, pacman_speed);
							/* Move toward the center of the turn_tile */
							struct v2 tile_center = add(scale(pacman_turn_tile, TILE_SIZE), TILE_CENTER);
							bool32 reached_centerline = FALSE;
							if (is_h_dir(level_data.pacman_dir)) {
								int mv_amt = tile_center.x - level_data.pacman_pos.x;
								if (mv_amt < 0) {
									level_data.pacman_pos.x -= MIN(-mv_amt, pacman_speed);
									reached_centerline = -mv_amt < pacman_speed;
								} else {
									level_data.pacman_pos.x += MIN(mv_amt, pacman_speed);
									reached_centerline = mv_amt < pacman_speed;
								}
							} else {
								int mv_amt = tile_center.y - level_data.pacman_pos.y;
								if (mv_amt < 0) {
									level_data.pacman_pos.y -= MIN(-mv_amt, pacman_speed);
									reached_centerline = -mv_amt < pacman_speed;
								} else {
									level_data.pacman_pos.y += MIN(mv_amt, pacman_speed);
									reached_centerline = mv_amt < pacman_speed;
								}
							}

							if (reached_centerline) { 
								level_data.pacman_turning = FALSE;
								level_data.pacman_dir = level_data.next_dir;
							}
						} else {
							struct v2 blocked_pos_s = level_data.pacman_pos;
							level_data.pacman_blocked = test_pacman_dir_blocked(level_data.pacman_dir, &blocked_pos_s);
							if (level_data.pacman_blocked) {
								blocked_tiles[num_blocked_tiles++] = pos_to_tile(&blocked_pos_s);
							} else {
								move_in_dir(level_data.pacman_dir, &level_data.pacman_pos, pacman_speed);
								if (level_data.pacman_pos.x < -TUNNEL_WIDTH / 2) {
									level_data.pacman_pos.x += ARENA_WIDTH + TUNNEL_WIDTH;
								} else if (level_data.pacman_pos.x >= ARENA_WIDTH + TUNNEL_WIDTH / 2) {
									level_data.pacman_pos.x -= ARENA_WIDTH + TUNNEL_WIDTH;
								}
							}
						}
					}

					{
						struct v2 pacman_tile = pos_to_tile(&level_data.pacman_pos);
						char ch = dot_map[pacman_tile.y][pacman_tile.x];
						if (ch == '.') {
							level_data.pacman_eat_timer = 1;
							game_data.score += 10;
						} else if (ch == '*') {
							level_data.pacman_eat_timer = 3;
							level_data.pacman_powerup_timer = level_constants.pacman_powerup_time;
							level_data.consecutive_ghosts_eaten = 0;
							level_data.ghost_flash_timer = 0;

							reverse_ghosts(ghosts);
							int i;
							for (i = 0; i < NUM_GHOSTS; ++i) {
								if (ghosts[i].mode != EYES) {
									ghosts[i].mode = FRIGHTENED;
								}
							}

							game_data.score += 50;
						}
						if (ch == '.' || ch == '*') {
							uint32 fruit_dot_counter = 70;
							level_data.dot_timer = 0;
							++level_data.dots_eaten;
							if (level_data.dots_eaten == fruit_dot_counter) {
								level_data.fruit_is_visible = TRUE;
							} else if (level_data.dots_eaten == NUM_DOTS) {
								game_mode = LEVEL_TRANSITION;
								transition_timer = 0;
							}

							if (level_data.use_global_ghost_house_dot_counter) {
								++level_data.global_ghost_house_dot_counter;
							} else {
								int i;
								for (i = PINKY; i < NUM_GHOSTS; ++i) {
									if (ghosts[i].ghost_house_state == IN_GHOST_HOUSE) {
										++ghosts[i].dot_counter;
									}
								}
							}
						} else {
							++level_data.dot_timer;
						}
						dot_map[pacman_tile.y][pacman_tile.x] = 0;
					}

					if (level_data.num_ghost_mode_cycles < 4 && !level_data.pacman_powerup_timer && !debug_ghost_mode_overridden) {
						++level_data.ghost_mode_timer;

						if (!level_data.is_chasing && level_data.ghost_mode_timer > level_constants.scatter_times[level_data.num_ghost_mode_cycles]) {
							/* Switch to chase mode */
							level_data.is_chasing = TRUE;
							level_data.ghost_mode_timer = 0;
							++level_data.num_ghost_mode_cycles;
							reverse_ghosts(ghosts);
						} else if (level_data.is_chasing && level_data.ghost_mode_timer > level_constants.chase_times[level_data.num_ghost_mode_cycles]) {
							/* Switch to scatter mode */
							level_data.is_chasing = FALSE;
							level_data.ghost_mode_timer = 0;
							reverse_ghosts(ghosts);
						}
					}

					{ /* Update all ghost positions */
						int i;
						for (i = 0; i < NUM_GHOSTS; ++i) {
							struct ghost* ghost = &ghosts[i];
							if (ghost->mode != EYES && level_data.ghost_eat_timer) {
								continue;
							}
							uint32 speed;
							struct v2 eyes_target_tile = { ARENA_WIDTH_IN_TILES / 2 - 1, 11 };
							struct v2 eyes_target_pos = add(scale(eyes_target_tile, TILE_SIZE), TILE_CENTER);

							if (ghost->ghost_house_state == EXITING_GHOST_HOUSE) {
								if (ghost->pos.x != eyes_target_pos.x) {
									ghost->dir = ghost->pos.x < eyes_target_pos.x ? RIGHT : LEFT;
								} else {
									ghost->dir = UP;
								}
							}
							{
								uint32 ghost_eyes_speed = get_speed(175);
								uint32 elroy_v2_speed = level_constants.elroy_v1_speed + get_speed(5);
								uint32 elroy_v2_dots_left = level_constants.elroy_v1_dots_left / 2;

								struct v2 ghost_tile = pos_to_tile(&ghost->pos);
								/* If the ghost is inside the tunnel, slow down. */
								bool32 is_in_tunnel = ghost_tile.y == 14 && (ghost_tile.x <= 5 || ghost_tile.x >= ARENA_WIDTH_IN_TILES - 5);
								if (ghost->mode == EYES) {
									speed = ghost_eyes_speed;
								} else if (ghost->ghost_house_state != OUTSIDE_GHOST_HOUSE) {
									speed = level_constants.ghost_tunnel_speed;
								} else if (is_in_tunnel) {
									speed = level_constants.ghost_tunnel_speed;
								} else if (ghost->mode == FRIGHTENED) {
									speed = level_constants.ghost_frightened_speed;
								} else if (i == BLINKY && NUM_DOTS - level_data.dots_eaten <= level_constants.elroy_v1_dots_left) {
									/* Oh he mad now beotch. */
									speed = level_constants.elroy_v1_speed;
								} else if (i == BLINKY && NUM_DOTS - level_data.dots_eaten <= elroy_v2_dots_left) {
									/* Cruise Elroy */
									speed = elroy_v2_speed;
								} else {
									speed = level_constants.ghost_speed;
								}
								if (ghost->ghost_house_state == EXITING_GHOST_HOUSE) {
									int offset = is_h_dir(ghost->dir) ?
										eyes_target_pos.x - ghost->pos.x :
										eyes_target_pos.y - ghost->pos.y
										;
									if (offset < 0) {
										offset = -offset;
									}
									speed = MIN(offset, speed);
								} else if (!eql(&ghost_tile, &ghost->last_tile)) {
									struct v2 tile_center = add(scale(ghost_tile, TILE_SIZE), TILE_CENTER);
									bool32 making_a_90_degree_turn = ghost->is_path_chosen && is_h_dir(ghost->chosen_dir) != is_h_dir(ghost->dir);
									if (making_a_90_degree_turn) {
										int offset = is_h_dir(ghost->dir) ?
											tile_center.x - ghost->pos.x :
											tile_center.y - ghost->pos.y
											;
										if (offset < 0) {
											offset = -offset;
										}
										speed = MIN(offset, speed);
									}
								}
							}

							move_in_dir(ghost->dir, &ghost->pos, speed);
							if (ghost->pos.x < -TUNNEL_WIDTH / 2) {
								ghost->pos.x += ARENA_WIDTH + TUNNEL_WIDTH;
							} else if (ghost->pos.x >= ARENA_WIDTH + TUNNEL_WIDTH / 2) {
								ghost->pos.x -= ARENA_WIDTH + TUNNEL_WIDTH;
							}
							struct v2 ghost_tile = pos_to_tile(&ghost->pos);

							if (ghost->ghost_house_state == EXITING_GHOST_HOUSE) {
								struct v2 just_below_eyes_tile = eyes_target_tile;
								--just_below_eyes_tile.y;
								if (eql(&eyes_target_pos, &ghost->pos)) {
									ghost->ghost_house_state = OUTSIDE_GHOST_HOUSE;
								} else if (!eql(&ghost_tile, &just_below_eyes_tile)) {
									continue;
								}
							}

							if (!eql(&ghost_tile, &ghost->last_tile)) {
								/* CLEANUP: merge with other movement to center */
								struct v2 tile_center = add(scale(ghost_tile, TILE_SIZE), TILE_CENTER);
								bool32 made_it_to_center;
								switch (ghost->dir) {
									case UP:
										made_it_to_center = ghost->pos.y <= tile_center.y;
										break;
									case DOWN:
										made_it_to_center = ghost->pos.y >= tile_center.y;
										break;
									case LEFT:
										made_it_to_center = ghost->pos.x <= tile_center.x;
										break;
									case RIGHT:
										made_it_to_center = ghost->pos.x >= tile_center.x;
										break;
								}
								if (!made_it_to_center) {
									continue;
								}

								ghost->last_tile = ghost_tile;
								if (ghost->is_path_chosen) {
									ghost->dir = ghost->chosen_dir;
									ghost->is_path_chosen = FALSE;
								}

								move_in_dir(ghost->dir, &ghost_tile, 1);
								enum dir paths[NUM_DIRS];
								int num_paths = 0;
								/* Get all paths from the next tile. */
								{
									int dir;
									for (dir = 0; dir < NUM_DIRS; ++dir) {
										/* Prevent reversal */
										if ((is_h_dir(dir) == is_h_dir(ghost->dir) && dir != ghost->dir)) {
											continue;
										}
										/* Only horizontal dirs when leaving the ghost house */
										if (ghost->ghost_house_state == EXITING_GHOST_HOUSE && !is_h_dir(dir)) {
											continue;
										}

										struct v2 tile = ghost_tile;
										move_in_dir(dir, &tile, 1);

										bool32 ghost_is_blocked = !!strchr(
												ghost->mode == EYES ?
												"/`[]-|+" : "/`[]-|+_",
												arena_get(tile.y, tile.x));
										if (ghost_is_blocked) {
											blocked_tiles[num_blocked_tiles++] = tile;
										} else {
											/* Don't go through any of the 'forbidden tiles'. */
											if (dir == UP && ghost->mode == NORMAL) {
												int i;
												bool32 found = FALSE;
												for_array(i, forbidden_upward_tiles) {
													if (eql(&forbidden_upward_tiles[i], &tile)) {
														found = TRUE;
														break;
													}
												}
												if (found) {
													continue;
												}
											}
											paths[num_paths++] = dir;
										}
									}
								}
								assert(num_paths >= 1);

								int best_choice;

								struct v2 scatter_targets[NUM_GHOSTS] = {
									{ ARENA_WIDTH_IN_TILES - 3, -3 },
									{ 2, -3 },
									{ ARENA_WIDTH_IN_TILES, ARENA_HEIGHT_IN_TILES },
									{ 0, ARENA_HEIGHT_IN_TILES },
								};
								if (ghost->mode == FRIGHTENED && ghost->ghost_house_state == OUTSIDE_GHOST_HOUSE) {
									/* Take pseudo-random turns. */
									int choice = rand_r(&level_data.fright_ghost_seed) % NUM_DIRS;
									best_choice = -1;
									while (best_choice == -1) {
										int i;
										for (i = 0; i < num_paths; ++i) {
											if (paths[i] == choice) {
												best_choice = choice;
												break;
											}
										}
										if (best_choice == -1) {
											--choice;
											if (choice < 0) {
												choice = NUM_DIRS - 1;
											}
										}
									}
								} else {
									/* Chase after a target */
									if (ghost->ghost_house_state == IN_GHOST_HOUSE) {
										/* BOUNCING, alternate targets. */
										uint32 ghost_global_dot_limits[] = { 0, 7, 17, 32 };
										if (ghost_tile.y >= 13 && ghost->mode == EYES) {
											ghost->mode = NORMAL;
										}
										enum ghost_name next_ghost_released;
										for (next_ghost_released = PINKY; next_ghost_released < NUM_GHOSTS; ++next_ghost_released) {
											if (ghosts[next_ghost_released].ghost_house_state == IN_GHOST_HOUSE) {
												break;
											}
										}
										if (ghost->name == BLINKY ||
												(ghost->name == next_ghost_released && (
													level_data.dot_timer >= seconds_to_frames(4) ||
													(level_data.use_global_ghost_house_dot_counter && level_data.global_ghost_house_dot_counter >= ghost_global_dot_limits[i]) ||
													(!level_data.use_global_ghost_house_dot_counter && ghost->dot_counter >= level_constants.ghost_dot_limits[i])))) {
											if (ghost->name != BLINKY) {
												level_data.dot_timer = 0;
											}
											ghost->target_tile = top_house_targets[PINKY];
											ghost->ghost_house_state = EXITING_GHOST_HOUSE;
										} else if (ghost->bounce_target_bottom) {
											if (eql(&ghost_tile, &bottom_house_targets[i])) {
												ghost->bounce_target_bottom = !ghost->bounce_target_bottom;
												ghost->target_tile = top_house_targets[i];
											} else {
												ghost->target_tile = bottom_house_targets[i];
											}
										} else {
											if (eql(&ghost_tile, &top_house_targets[i])) {
												ghost->bounce_target_bottom = !ghost->bounce_target_bottom;
												ghost->target_tile = bottom_house_targets[i];
											} else {
												ghost->target_tile = top_house_targets[i];
											}
										}
									} else if (ghost->mode == NORMAL) {
										get_normal_target_tile(&level_data, scatter_targets, &ghosts[BLINKY].pos, ghost, ghost_tile);
									} else if (ghost->mode == EYES) {
										/* Target the door. */
										struct v2 home_target = { ARENA_WIDTH_IN_TILES / 2 - 1, 13 };
										if (eql(&ghost_tile, &eyes_target_tile)) {
											ghost->ghost_house_state = ENTERING_GHOST_HOUSE;
											ghost->target_tile = home_target;
										} else if (ghost->ghost_house_state == ENTERING_GHOST_HOUSE) {
											if (!eql(&ghost_tile, &home_target)) {
												ghost->target_tile = home_target;
											} else {
												ghost->ghost_house_state = IN_GHOST_HOUSE;
												ghost->target_tile = home_target;
												++ghost->target_tile.y;
											}
										} else {
											ghost->target_tile = eyes_target_tile;
										}
									}

									int i, best_path = 0, best_distance = -1;
									for (i = 0; i < num_paths; ++i) {
										struct v2 tile = ghost_tile;
										move_in_dir(paths[i], &tile, 1);
										int distance = DIST_SQUARE(tile.x - ghost->target_tile.x, tile.y - ghost->target_tile.y);
										if (best_distance == -1 || distance < best_distance) {
											best_distance = distance;
											best_path = i;
										}
									}	
									best_choice = paths[best_path];
								}

								ghost->is_path_chosen = TRUE;
								ghost->chosen_dir = best_choice;
							}
						}
					}

					{
						struct v2 pacman_tile = pos_to_tile(&level_data.pacman_pos);

						if (level_data.fruit_is_visible && (eql(&pacman_tile, &fruit_tile))) {
							level_data.fruit_is_visible = FALSE;
							level_data.fruit_score_timer = seconds_to_frames(2);
							game_data.score += symbol_points[level_constants.bonus_symbol];
						}

						/* Check for pacman/ghost collisions */
						int i;
						for (i = 0; i < NUM_GHOSTS; ++i) {
							struct v2 ghost_tile = pos_to_tile(&ghosts[i].pos);
							if (eql(&ghost_tile, &pacman_tile)) {
								if (ghosts[i].mode == FRIGHTENED && !level_data.ghost_to_be_eaten) {
									uint32 ghost_eat_time = seconds_to_frames(1);
									level_data.ghost_eat_timer = ghost_eat_time;

									game_data.score += get_ghost_score(level_data.consecutive_ghosts_eaten++);
									level_data.ghost_to_be_eaten = &ghosts[i];
								} else if (ghosts[i].mode == NORMAL && !debug_no_death_mode) {
									game_mode = LOSING_A_LIFE;
									level_data.extra_life_timer = 0;
									level_data.ghost_eat_timer = 0;
									transition_timer = 0;
								}
							}
						}
					}

					high_score = MAX(game_data.score, high_score);
					if (old_score / 10000 != game_data.score / 10000) {
						add_extra_life(&game_data, &level_data);
					}
					if (level_data.extra_life_timer) {
						--level_data.extra_life_timer;
					}
				} break;
				case GAME_OVER: {
				} break;
				case LEVEL_TRANSITION: {
					++transition_timer;
					if (transition_timer > 30) {
						set_level_constants(&level_constants, ++game_data.current_level);
						start_new_level(ghosts, &level_data, game_data.original_fright_ghost_seed);
						transition_timer = 0;
						game_mode = PAUSED_BEFORE_PLAYING;
					}
				} break;
				case LOSING_A_LIFE: {
					++transition_timer;
					switch (transition_timer) {
						/* Spin in a circle */
						case LIFE_LOST_PAUSE_TIME + LIFE_LOST_TURN_TIME*1: 
						case LIFE_LOST_PAUSE_TIME + LIFE_LOST_TURN_TIME*2: 
						case LIFE_LOST_PAUSE_TIME + LIFE_LOST_TURN_TIME*3: 
						case LIFE_LOST_PAUSE_TIME + LIFE_LOST_TURN_TIME*4: 
							--level_data.pacman_dir;
							if (level_data.pacman_dir == -1) {
								level_data.pacman_dir = NUM_DIRS - 1;
							}
							break;
						/* Restart the level */
						case LIFE_LOST_PAUSE_TIME + LIFE_LOST_TURN_TIME*8:
							if (game_data.num_extra_lives) {
								--game_data.num_extra_lives;
								transition_timer = 0;
								game_mode = PAUSED_BEFORE_PLAYING;

								return_ghosts_and_pacman_to_start_position(ghosts, &level_data);
								level_data.use_global_ghost_house_dot_counter = TRUE;
								level_data.global_ghost_house_dot_counter = 0;
								level_data.ghost_mode_timer = 0;
								level_data.num_ghost_mode_cycles = 0;
								level_data.is_chasing = FALSE;
							} else {
								game_mode = GAME_OVER;
							}
							break;
					}
				} break;
			}

			erase();
			view.left = 2;
			view.top = 3;

			if (view.zoom_view) {
				struct v2 offset = { -8, -2 };
				view.camera_target_tile = add(offset, pos_to_tile(&ghosts[INKY].pos));
			} else {
				struct v2 zero = { 0 };
				view.camera_target_tile = zero;
			}
			if (level_data.extra_life_timer && level_data.extra_life_timer / 10 % 2 == 0) {
				init_pair(WALL_PAIR, COLOR_LIGHT_BLUE, COLOR_DARK_BACKGROUND);
			} else {
				init_pair(WALL_PAIR, COLOR_WALL_BLUE, COLOR_DARK_BACKGROUND);
			}
			{ /* Draw Arena */
				int row, col;

				for (row = 0; row < ARENA_HEIGHT_IN_TILES; ++row) {
					for (col = 0; col < ARENA_WIDTH_IN_TILES; ++col) {
						char ch = arena_get(row, col), draw_ch = ch, fill_ch = ' ';
						switch (ch) {
							case '`':
							case '[':
							case ']':
							case '/':
								draw_ch = '+';
								attron(COLOR_PAIR(WALL_PAIR));
								break;
							case 'x':
								draw_ch = ' ';
								attron(COLOR_PAIR(WALL_PAIR));
								break;
							case ' ':
								attron(COLOR_PAIR(EMPTY_PAIR));
								break;
							case '_':
								attron(COLOR_PAIR(DOOR_PAIR));
								fill_ch = ch;
								break;
							case '-':
							case '|':
							case '+':
								attron(COLOR_PAIR(WALL_PAIR));
								break;
						}
						if (ch == '-') {
							fill_ch = ch;
						}
						draw_tile(row, col, draw_ch, &view, fill_ch);
					}
				}
			}

			if (level_data.fruit_is_visible) {
				draw_fruit(fruit_tile, level_constants.bonus_symbol, &view);
			} else if (level_data.fruit_score_timer) {
				enable_bonus_symbol_color(level_constants.bonus_symbol);
				--level_data.fruit_score_timer;
				struct v2 pos = draw_pos_v2(fruit_tile, &view);
				mvprintw(pos.y, pos.x, "%u", symbol_points[level_constants.bonus_symbol]);
			}
			{
				int i;
				int start_level = game_data.current_level - 6;
				start_level = MAX(0, start_level);
				for (i = start_level; i < game_data.current_level + 1; ++i) {
					struct v2 tile = { ARENA_WIDTH_IN_TILES - 2*(i+1 - start_level), ARENA_HEIGHT_IN_TILES };
					draw_fruit(tile, get_symbol_for_level(i), &view);
				}
			}

			{
				attron(COLOR_PAIR(DOT_PAIR));
				int row, col;
				for (row = 0; row < ARENA_HEIGHT_IN_TILES; ++row) {
					for (col = 0; col < ARENA_WIDTH_IN_TILES; ++col) {
						char ch = dot_map[row][col];
						if (ch == '.' || ch == '*') {
							draw_tile(row, col, ch, &view, ' ');
						}
					}
				}
			}

			attron(COLOR_PAIR(PACMAN_PAIR));

			char pacman_char = 'O';
			if (game_mode != PLAYING || level_data.pacman_blocked || level_data.pacman_chomp_timer / 3 % 3 != 0) {
				switch (level_data.pacman_dir) {
					case RIGHT:
						pacman_char = '<';
						break;
					case LEFT:
						pacman_char = '>';
						break;
					case UP:
						pacman_char = 'Y';
						break;
					case DOWN:
						pacman_char = '^';
						break;
				}
			}

			{
				int i;
				for (i = 0; i < game_data.num_extra_lives; ++i) {
					draw_tile(ARENA_HEIGHT_IN_TILES, i + 1, '<', &view, ' ');
				}
			}

			struct v2 pacman_tile = pos_to_tile(&level_data.pacman_pos);
			if (!level_data.ghost_eat_timer) {
				if (pacman_tile.x >= 0 && pacman_tile.x < ARENA_WIDTH_IN_TILES) {
					draw_tile_v2(pacman_tile, pacman_char, &view, ' ');
				}
			}

#if 0
			{
				attron(COLOR_PAIR(DOOR_PAIR));
				int i;
				for (i = 0; i < num_blocked_tiles; ++i) {
					draw_tile_v2(blocked_tiles[i], 'X', &view);
				}

				for_array(i, forbidden_upward_tiles) {
					draw_tile_v2(forbidden_upward_tiles[i], 'X', &view);
				}
			}
#endif

			if (game_mode != LOSING_A_LIFE && game_mode != GAME_OVER) {
				int i;
				for (i = 0; i < NUM_GHOSTS; ++i) {
					if (level_data.ghost_eat_timer && &ghosts[i] == level_data.ghost_to_be_eaten) {
						continue;
					}
					if (ghosts[i].mode == FRIGHTENED) {
						if (level_data.ghost_flash_timer / GHOST_FLASH_TIME % 2 == 0) {
							attron(COLOR_PAIR(FRIGHT_PAIR));
						} else {
							attron(COLOR_PAIR(FRIGHT_FLASH_PAIR));
						}
					} else if (ghosts[i].mode == EYES) {
						attron(COLOR_PAIR(EYES_PAIR));
					} else {
						attron(COLOR_PAIR(ghosts[i].curses_color_pair));
					}
					struct v2 ghost_tile = pos_to_tile(&ghosts[i].pos);
					if (ghost_tile.x >= 0 && ghost_tile.x < ARENA_WIDTH_IN_TILES) {
						draw_tile_v2(ghost_tile, 'm', &view, ' ');
					}

#if 0
					/* Draw Target Tile */
					attron(COLOR_PAIR(DOT_PAIR));
					draw_tile_v2(ghosts[i].target_tile, ghosts[i].nickname, &view);
#endif
				}
			}

			if (level_data.ghost_eat_timer) {
				attron(COLOR_PAIR(KEY_PAIR));
				struct v2 pos = draw_pos_v2(pacman_tile, &view);
				mvprintw(pos.y, pos.x, "%d", get_ghost_score(level_data.consecutive_ghosts_eaten - 1));
			}

			if (game_mode == GAME_OVER) {
				attron(COLOR_PAIR(GAME_OVER_TEXT_PAIR));
				struct v2 pos = draw_pos(HOUSE_BOTTOM + 2, HOUSE_LEFT, &view);
				mvprintw(pos.y, pos.x, "G A M E    O V E R");
			}


			if (view.zoom_view) {
				attron(COLOR_PAIR(PACMAN_PAIR));
				draw_tile_v2(pacman_tile, 'X', &view, ' ');

				if (level_data.pacman_turning) {
					attron(COLOR_PAIR(WALL_PAIR));
					draw_tile_v2(pacman_turn_tile, 'O', &view, ' ');
				}

				attron(COLOR_PAIR(BG_PAIR));
				mvaddch(
						ghosts[INKY].pos.y / PIXEL_SIZE + view.top - view.camera_target_tile.y * TILE_SIZE_IN_PIXELS,
						ghosts[INKY].pos.x / PIXEL_SIZE + view.left - view.camera_target_tile.x * TILE_SIZE_IN_PIXELS, 'O');
			}

			attron(COLOR_PAIR(BG_PAIR));
			mvprintw(view.top - 2, view.left + 2*3, "1 U P");
			mvprintw(view.top - 1, view.left + 2*3, "%7d",  game_data.score);
			mvprintw(view.top - 2, (HOUSE_CENTER - 2)*3, "H I G H   S C O R E");
			mvprintw(view.top - 1, (HOUSE_CENTER - 1)*3, "%7d",  game_data.score);
#if 0
			attron(COLOR_PAIR(BG_PAIR));
			int diag_row = 0;
			mvprintw(diag_row++, 0, "Score: %06d, High Score: %06d",  game_data.score, high_score);
			mvprintw(diag_row++, 0, "Dots Eaten: %d",  level_data.dots_eaten);
			mvprintw(diag_row++, 0, "Current Level %u%s",  game_data.current_level, debug_no_death_mode ? ", GOD-MODE" : "");
#endif

			refresh();

			frame_timer = 0;
		}
		usleep(1);
	}

	endwin();

	return 0;
}
