const { supabase } = require('../config/database');
const ApiResponse = require('../utils/response');

class EventController {
  getEvents = async (req, res) => {
    try {
      const { status = 'upcoming', limit = 20, offset = 0 } = req.query;
      const now = new Date().toISOString();

      let query = supabase
        .from('events')
        .select('*', { count: 'exact' })
        .order('start_time', { ascending: true });

      // Filter by status
      if (status === 'upcoming') {
        query = query.gte('start_time', now);
      } else if (status === 'ongoing') {
        query = query.lte('start_time', now).gte('end_time', now);
      } else if (status === 'past') {
        query = query.lt('end_time', now);
      }

      const { data: events, error, count } = await query
        .range(offset, offset + limit - 1);

      if (error) {
        return ApiResponse.error(res, `Failed to fetch events: ${error.message}`, 400);
      }

      // Add status field to each event
      const eventsWithStatus = events?.map(event => ({
        ...event,
        status: this.getEventStatus(event.start_time, event.end_time)
      })) || [];

      return ApiResponse.success(res, {
        events: eventsWithStatus,
        pagination: {
          total: count || 0,
          limit: parseInt(limit),
          offset: parseInt(offset),
          has_more: (offset + events?.length) < (count || 0)
        }
      }, 'Events retrieved successfully');

    } catch (error) {
      console.error('Get events error:', error);
      return ApiResponse.serverError(res, `Internal server error during fetching events: ${error.message}`);
    }
  }

  getEvent = async (req, res) => {
    try {
      const { eventId } = req.params;

      if (!eventId) {
        return ApiResponse.error(res, 'Event ID is required', 400);
      }

      const { data: event, error } = await supabase
        .from('events')
        .select('*')
        .eq('id', eventId)
        .single();

      if (error || !event) {
        return ApiResponse.notFound(res, 'Event not found');
      }

      const eventWithStatus = {
        ...event,
        status: this.getEventStatus(event.start_time, event.end_time),
        participants_count: 0 // You'd need to query event_participants table
      };

      return ApiResponse.success(res, eventWithStatus, 'Event retrieved successfully');

    } catch (error) {
      console.error('Get event error:', error);
      return ApiResponse.serverError(res, `Internal server error during fetching event: ${error.message}`);
    }
  }

  createEvent = async (req, res) => {
    try {
      const { title, description, start_time, end_time, image_url, rewards } = req.body;

      // Check admin role (you need to implement role checking)
      // if (req.user.role !== 'admin') {
      // return ApiResponse.forbidden(res, 'Only admins can create events');
      // }

      if (!title || !description || !start_time || !end_time) {
        return ApiResponse.error(res, 'Title, description, start_time, and end_time are required', 400);
      }

      // Validate dates
      const startDate = new Date(start_time);
      const endDate = new Date(end_time);

      if (startDate >= endDate) {
        return ApiResponse.error(res, 'End time must be after start time', 400);
      }

      const eventData = {
        title,
        description,
        start_time: startDate.toISOString(),
        end_time: endDate.toISOString(),
        image_url: image_url || null,
        rewards: rewards || [],
        created_at: new Date().toISOString()
      };

      const { data: event, error } = await supabase
        .from('events')
        .insert([eventData])
        .select()
        .single();

      if (error) {
        return ApiResponse.error(res, `Failed to create event: ${error.message}`, 400);
      }

      return ApiResponse.created(res, event, 'Event created successfully');

    } catch (error) {
      console.error('Create event error:', error);
      return ApiResponse.serverError(res, `Internal server error during creating event: ${error.message}`);
    }
  }

  getActiveEvents = async (req, res) => {
    try {
      const now = new Date().toISOString();

      const { data: events, error } = await supabase
        .from('events')
        .select('*')
        .lte('start_time', now)
        .gte('end_time', now)
        .order('start_time', { ascending: true });

      if (error) {
        return ApiResponse.error(res, `Failed to fetch active events: ${error.message}`, 400);
      }

      // Format for Unreal Engine
      const formattedEvents = events?.map(event => ({
        id: event.id,
        title: event.title,
        description: event.description,
        start_time: event.start_time,
        end_time: event.end_time,
        image_url: event.image_url,
        rewards: event.rewards
      })) || [];

      return ApiResponse.success(res, formattedEvents, 'Active events retrieved successfully');

    } catch (error) {
      console.error('Get active events error:', error);
      return ApiResponse.serverError(res, `Internal server error during fetching active events: ${error.message}`);
    }
  }

  joinEvent = async (req, res) => {
    try {
      const { eventId } = req.params;
      const userId = req.user.id;

      if (!eventId) {
        return ApiResponse.error(res, 'Event ID is required', 400);
      }

      // Check if event exists and is active
      const { data: event } = await supabase
        .from('events')
        .select('*')
        .eq('id', eventId)
        .single();

      if (!event) {
        return ApiResponse.notFound(res, 'Event not found');
      }

      const now = new Date();
      const startTime = new Date(event.start_time);
      const endTime = new Date(event.end_time);

      if (now < startTime) {
        return ApiResponse.error(res, 'Event has not started yet', 400);
      }

      if (now > endTime) {
        return ApiResponse.error(res, 'Event has ended', 400);
      }

      // Check if already joined
      const { data: existingJoin } = await supabase
        .from('event_participants')
        .select('id')
        .eq('event_id', eventId)
        .eq('user_id', userId)
        .single();

      if (existingJoin) {
        return ApiResponse.conflict(res, 'Already joined this event');
      }

      // Join event
      const { error: joinError } = await supabase
        .from('event_participants')
        .insert({
          event_id: eventId,
          user_id: userId,
          joined_at: new Date().toISOString()
        });

      if (joinError) {
        return ApiResponse.error(res, 'Failed to join event', 500);
      }

      return ApiResponse.success(res, { success: true }, 'Joined event successfully');

    } catch (error) {
      console.error('Join event error:', error);
      return ApiResponse.serverError(res, `Internal server error during joining event: ${error.message}`);
    }
  }

  getEventStatus = (startTime, endTime) => {
    const now = new Date();
    const start = new Date(startTime);
    const end = new Date(endTime);

    if (now < start) return 'upcoming';
    if (now >= start && now <= end) return 'ongoing';
    return 'past';
  }
}

module.exports = new EventController();