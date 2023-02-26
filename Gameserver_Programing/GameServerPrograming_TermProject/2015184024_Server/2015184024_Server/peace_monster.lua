my_id = -1
hp = 100
att = 0
init_x = -1
init_y = -1


function set_npc_id(id)
   my_id = id
end

function set_npc_pos()
	init_x = math.random(0, 799)
	init_y = math.random(0, 799)
end


function event_player_move_notify (player_id, x, y)
   my_x = API_get_x_position(my_id)
   my_y = API_get_y_position(my_id)
   if x == my_x then
      if y == my_y then
         API_send_chat_packet(player_id, my_id, "Hello")
	  end
   end
end

