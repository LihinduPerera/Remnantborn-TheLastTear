const axios = require('axios');
const API_BASE = 'http://localhost:3000/api';
const HEALTH_BASE = 'http://localhost:3000';
async function testAPI() {
  console.log('🧪 Testing Game Backend API...\n');
  let token = null;
  let userId = null;
  let testUsername = null;
  let testPostId = null;
  let testEventId = null;
  // Helper function to make authenticated requests
  const authAxios = (method, url, data = null) => {
    return axios({
      method,
      url: `${API_BASE}${url}`,
      data,
      headers: { Authorization: `Bearer ${token}` }
    });
  };
  try {
    // 1. Test health endpoint
    console.log('1. Testing health endpoint...');
    const healthRes = await axios.get(`${HEALTH_BASE}/health`);
    console.log(`✅ Health: ${healthRes.data.status}\n`);
    // 2. Test database connection
    console.log('2. Testing database connection...');
    const dbRes = await axios.get(`${HEALTH_BASE}/db-status`);
    console.log(`✅ Database: ${dbRes.data.status}\n`);
    // 3. Test signup
    console.log('3. Testing signup...');
    const testUser = {
      email: `test${Date.now()}@example.com`,
      password: 'testpassword123',
      username: `testuser${Date.now()}`
    };
    const signupRes = await axios.post(`${API_BASE}/auth/signup`, testUser);
    token = signupRes.data.data.token;
    userId = signupRes.data.data.userId;
    testUsername = signupRes.data.data.username;
    console.log(`✅ Signup successful - User ID: ${userId}, Username: ${testUsername}\n`);
    // 4. Test login
    console.log('4. Testing login...');
    const loginRes = await axios.post(`${API_BASE}/auth/login`, {
      email: testUser.email,
      password: testUser.password
    });
    console.log(`✅ Login successful - Token received\n`);
    // 5. Test verify-token
    console.log('5. Testing verify-token...');
    const verifyRes = await authAxios('post', '/auth/verify-token');
    console.log(`✅ Token verified - Username: ${verifyRes.data.data.username}\n`);
    // 6. Test dev-login (only in development mode)
    console.log('6. Testing dev-login...');
    try {
      const devLoginRes = await axios.post(`${API_BASE}/auth/dev-login`, { email: testUser.email });
      console.log(`✅ Dev login successful - Username: ${devLoginRes.data.data.username}\n`);
    } catch (devErr) {
      console.log(`⚠️ Dev login skipped (may not be in development mode): ${devErr.message}\n`);
    }
    // 7. Test get profile
    console.log('7. Testing get profile...');
    const profileRes = await authAxios('get', `/profile/${userId}`);
    console.log(`✅ Profile retrieved - Level: ${profileRes.data.data.level}, Remnants: ${profileRes.data.data.remnant_count}\n`);
    // 8. Test update profile
    console.log('8. Testing update profile...');
    const updateProfileRes = await authAxios('put', `/profile/${userId}`, { bio: 'Test bio' });
    console.log(`✅ Profile updated\n`);
    // 9. Test update game stats
    console.log('9. Testing update game stats...');
    const updateStatsRes = await authAxios('patch', `/profile/${userId}/game-stats`, { level: 2, remnant_count: 200 });
    console.log(`✅ Game stats updated - New Level: ${updateStatsRes.data.data.level}\n`);
    // 10. Test get online status
    console.log('10. Testing get online status...');
    const statusRes = await authAxios('get', `/profile/${userId}/status`);
    console.log(`✅ Online status: ${statusRes.data.data.is_online ? 'Online' : 'Offline'}\n`);
    // 11. Test search users
    console.log('11. Testing search users...');
    const searchRes = await axios.get(`${API_BASE}/profile/search/users?query=${testUsername.substring(0, 5)}`);
    console.log(`✅ Users found: ${searchRes.data.data.count}\n`);
    // 12. Test get purchases
    console.log('12. Testing get purchases...');
    const purchasesRes = await authAxios('get', `/purchases/${userId}`);
    console.log(`✅ Purchases retrieved: ${purchasesRes.data.data.length}\n`);
    // 13. Test create purchase
    console.log('13. Testing create purchase...');
    const createPurchaseRes = await authAxios('post', '/purchases', {
      userId,
      item_type: 'skin',
      item_id: 'test-skin-1',
      price: 50
    });
    console.log(`✅ Purchase created - Item ID: ${createPurchaseRes.data.data.item_id}\n`);
    // 14. Test check ownership
    console.log('14. Testing check ownership...');
    const checkOwnershipRes = await authAxios('post', `/purchases/check-ownership/${userId}`, { item_ids: ['test-skin-1'] });
    console.log(`✅ Ownership checked - Owned: ${checkOwnershipRes.data.data.ownership['test-skin-1']}\n`);
    // 15. Test get purchase history
    console.log('15. Testing get purchase history...');
    const historyRes = await authAxios('get', `/purchases/history/${userId}`);
    console.log(`✅ History retrieved: ${historyRes.data.data.purchases.length}\n`);
    // 16. Test send message (private)
    console.log('16. Testing send message...');
    // Note: Need a second user for receiver_id. For testing, send to self or create another user.
    const sendMsgRes = await authAxios('post', '/chat/send', { receiver_id: userId, message: 'Test message' });
    console.log(`✅ Message sent - ID: ${sendMsgRes.data.data.chat_id}\n`);
    // 17. Test get messages
    console.log('17. Testing get messages...');
    const getMsgsRes = await authAxios('get', '/chat');
    console.log(`✅ Messages retrieved: ${getMsgsRes.data.data.messages.length}\n`);
    // 18. Test get conversations
    console.log('18. Testing get conversations...');
    const convosRes = await authAxios('get', '/chat/conversations');
    console.log(`✅ Conversations retrieved: ${convosRes.data.data.length}\n`);
    // 19. Test get channel messages
    console.log('19. Testing get channel messages...');
    const channelMsgsRes = await authAxios('get', '/chat/channel/public'); // Assuming 'public' channel
    console.log(`✅ Channel messages retrieved: ${channelMsgsRes.data.data.length}\n`);
    // 20. Test create post
    console.log('20. Testing create post...');
    const createPostRes = await authAxios('post', '/posts', { content: 'Test post content' });
    testPostId = createPostRes.data.data.id;
    console.log(`✅ Post created - ID: ${testPostId}\n`);
    // 21. Test get posts
    console.log('21. Testing get posts...');
    const getPostsRes = await axios.get(`${API_BASE}/posts`); // Optional auth
    console.log(`✅ Posts retrieved: ${getPostsRes.data.data.posts.length}\n`);
    // 22. Test like post
    console.log('22. Testing like post...');
    const likeRes = await authAxios('post', `/posts/${testPostId}/like`);
    console.log(`✅ Post liked - Likes: ${likeRes.data.data.likes}\n`);
    // 23. Test get single post
    console.log('23. Testing get single post...');
    const getPostRes = await axios.get(`${API_BASE}/posts/${testPostId}`);
    console.log(`✅ Post retrieved - Content: ${getPostRes.data.data.content.substring(0, 20)}...\n`);
    // 24. Test delete post
    // console.log('24. Testing delete post...');
    // const deletePostRes = await authAxios('delete', `/posts/${testPostId}`);
    // console.log(`✅ Post deleted\n`);
    // 25. Test get events
    console.log('25. Testing get events...');
    const eventsRes = await axios.get(`${API_BASE}/events`);
    console.log(`✅ Events retrieved: ${eventsRes.data.data.events.length}\n`);
    // 26. Test get active events
    console.log('26. Testing get active events...');
    const activeEventsRes = await axios.get(`${API_BASE}/events/active`);
    console.log(`✅ Active events retrieved: ${activeEventsRes.data.data.length}\n`);
    // 27. Test create event (admin only)
    console.log('27. Testing create event (admin only)...');
    try {
      const createEventRes = await authAxios('post', '/events', {
        title: 'Test Event',
        description: 'Test description',
        start_time: new Date(Date.now() - 3600000).toISOString(), // 1 hour ago
        end_time: new Date(Date.now() + 3600000).toISOString(), // 1 hour from now
      });
      testEventId = createEventRes.data.data.id;
      console.log(`✅ Event created - ID: ${testEventId}\n`);
    } catch (createEventErr) {
      console.log(`⚠️ Create event skipped (may require admin role): ${createEventErr.message}\n`);
    }
    // 28. Test get single event (use a sample event ID if create failed)
    console.log('28. Testing get single event...');
    const eventIdToGet = testEventId || 'some-existing-event-id'; // Replace with real ID if needed
    const getEventRes = await axios.get(`${API_BASE}/events/${eventIdToGet}`);
    console.log(`✅ Event retrieved - Title: ${getEventRes.data.data.title}\n`);
    // 29. Test join event
    console.log('29. Testing join event...');
    const joinEventRes = await authAxios('post', `/events/${eventIdToGet}/join`);
    console.log(`✅ Event joined\n`);

    // 30. Test create comment
    console.log('30. Testing create comment...');
    const createCommentRes = await authAxios('post', '/comments', {
      postId: testPostId,
      content: 'Great post! This is a test comment.'
    });
    const commentId = createCommentRes.data.data.id;
    console.log(`✅ Comment created - ID: ${commentId}\n`);

    // 31. Test get comments
    console.log('31. Testing get comments...');
    const getCommentsRes = await axios.get(`${API_BASE}/comments/post/${testPostId}`);
    console.log(`✅ Comments retrieved: ${getCommentsRes.data.data.comments.length}\n`);

    // 32. Test delete comment
    // console.log('32. Testing delete comment...');
    // await authAxios('delete', `/comments/${commentId}`);
    // console.log(`✅ Comment deleted\n`);

    // 33. Test send friend request (need another user ID - use a known one or create second user)
    console.log('33. Testing send friend request...');
    // For demo, assume a second user ID from DB or create one manually first
    // Replace with real friendId from your DB
    const secondUserId = '7ed23e99-fc7a-40f0-a6d6-35392dfe1c78'; // TODO: Create second user in test
    try {
      const requestRes = await authAxios('post', '/friends/request', { friendId: secondUserId });
      console.log(`✅ Friend request sent\n`);
      console.log('🎉 All tests completed!');
    } catch (e) {
      console.log(`⚠️ Friend request skipped (need second user): ${e.message}\n`);
    }
  } catch (error) {
    console.error('❌ Global test error:', error.message);
    if (error.response) {
      console.error('Response data:', error.response.data);
    }
  }
}
testAPI();