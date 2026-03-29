const axios = require('axios');

// Configuration for Unreal Engine testing
const API_BASE = 'http://localhost:3000/api';

async function testUnrealEndpoints() {
    console.log('🧪 Testing endpoints for Unreal Engine client...\n');
    
    // Test credentials
    const testCredentials = {
        email: 'unreal_test@example.com',
        password: 'unreal123',
        username: 'UnrealPlayer'
    };
    
    let token = null;
    let userId = null;

    try {
        // 1. Clean up any existing test user
        console.log('1. Cleaning up old test user...');
        try {
            const { data: users } = await axios.get(`${API_BASE}/profile/search/users?query=UnrealPlayer`);
            if (users.data && users.data.users.length > 0) {
                console.log('⚠️ Test user already exists');
            }
        } catch (e) {
            // Ignore search errors
        }

        // 2. Test signup endpoint
        console.log('\n2. Testing signup endpoint...');
        try {
            const signupRes = await axios.post(`${API_BASE}/auth/signup`, testCredentials);
            console.log(`✅ Signup successful`);
            console.log(`   User ID: ${signupRes.data.data.userId}`);
            console.log(`   Username: ${signupRes.data.data.username}`);
            console.log(`   Level: ${signupRes.data.data.level}`);
            console.log(`   Remnant Count: ${signupRes.data.data.remnant_count}`);
            
            token = signupRes.data.data.token;
            userId = signupRes.data.data.userId;
        } catch (error) {
            if (error.response?.data?.message?.includes('already')) {
                console.log('⚠️ User already exists, trying login instead...');
                
                // 3. Test login endpoint
                console.log('\n3. Testing login endpoint...');
                const loginRes = await axios.post(`${API_BASE}/auth/login`, {
                    email: testCredentials.email,
                    password: testCredentials.password
                });
                
                console.log(`✅ Login successful`);
                console.log(`   User ID: ${loginRes.data.data.userId}`);
                console.log(`   Username: ${loginRes.data.data.username}`);
                
                token = loginRes.data.data.token;
                userId = loginRes.data.data.userId;
            } else {
                throw error;
            }
        }

        // 4. Test verify-token endpoint
        console.log('\n4. Testing verify-token endpoint...');
        const verifyRes = await axios.post(`${API_BASE}/auth/verify-token`, {}, {
            headers: { Authorization: `Bearer ${token}` }
        });
        console.log(`✅ Token verification successful`);
        console.log(`   Username: ${verifyRes.data.data.username}`);
        console.log(`   Email: ${verifyRes.data.data.email}`);

        // 5. Test update game stats endpoint
        console.log('\n5. Testing update game stats endpoint...');
        const updateRes = await axios.patch(`${API_BASE}/profile/${userId}/game-stats`, {
            level: 5,
            remnant_count: 500,
            operation: 'set'
        }, {
            headers: { Authorization: `Bearer ${token}` }
        });
        console.log(`✅ Game stats updated successfully`);
        console.log(`   New Level: ${updateRes.data.data.level}`);
        console.log(`   New Remnant Count: ${updateRes.data.data.remnant_count}`);

        // 6. Test get profile endpoint
        console.log('\n6. Testing get profile endpoint...');
        const profileRes = await axios.get(`${API_BASE}/profile/${userId}`, {
            headers: { Authorization: `Bearer ${token}` }
        });
        console.log(`✅ Profile retrieved successfully`);
        console.log(`   Bio: ${profileRes.data.data.bio || 'Not set'}`);
        console.log(`   Last Active: ${profileRes.data.data.last_active}`);

        // 7. Test update profile endpoint
        console.log('\n7. Testing update profile endpoint...');
        const updateProfileRes = await axios.put(`${API_BASE}/profile/${userId}`, {
            bio: 'Unreal Engine 5 Player'
        }, {
            headers: { Authorization: `Bearer ${token}` }
        });
        console.log(`✅ Profile updated successfully`);

        // 8. Test search users endpoint (no auth required)
        console.log('\n8. Testing search users endpoint...');
        const searchRes = await axios.get(`${API_BASE}/profile/search/users?query=Unreal`);
        console.log(`✅ Search completed`);
        console.log(`   Found ${searchRes.data.data.count} users`);

        // 9. Test create purchase endpoint
        console.log('\n9. Testing create purchase endpoint...');
        const purchaseRes = await axios.post(`${API_BASE}/purchases`, {
            userId: userId,
            item_type: 'skin',
            item_id: 'dragon_skin_001',
            price: 150
        }, {
            headers: { Authorization: `Bearer ${token}` }
        });
        console.log(`✅ Purchase completed successfully`);
        console.log(`   Item: ${purchaseRes.data.data.item_type}/${purchaseRes.data.data.item_id}`);

        // 10. Test match complete endpoint
        console.log('\n10. Testing match complete endpoint...');
        const matchId = `test-match-${Date.now()}`;
        const matchCompleteRes = await axios.post(`${API_BASE}/match/complete`, {
            matchId,
            mapName: 'TestGround',
            gameMode: 'MultiplayerGameMode',
            durationSeconds: 180,
            participants: [
                {
                    userId,
                    playerName: testCredentials.username,
                    playerId: 1,
                    characterId: 'Warrior_01',
                    placement: 1,
                    eliminationOrder: 0,
                    survivalTimeSeconds: 180,
                    isWinner: true,
                    isAliveAtEnd: true,
                    killCount: 2,
                    deathCount: 0,
                    damageDealt: 420,
                    damageTaken: 120
                }
            ]
        }, {
            headers: { Authorization: `Bearer ${token}` }
        });
        console.log(`✅ Match complete submitted`);
        console.log(`   Match ID: ${matchCompleteRes.data.data.match_id}`);
        console.log(`   Participants saved: ${matchCompleteRes.data.data.participants_saved}`);

        // 11. Test check ownership endpoint
        console.log('\n11. Testing check ownership endpoint...');
        const ownershipRes = await axios.post(`${API_BASE}/purchases/check-ownership/${userId}`, {
            item_ids: ['dragon_skin_001', 'phoenix_skin_001']
        }, {
            headers: { Authorization: `Bearer ${token}` }
        });
        console.log(`✅ Ownership check completed`);
        console.log(`   Owns dragon_skin_001: ${ownershipRes.data.data.ownership.dragon_skin_001}`);
        console.log(`   Owns phoenix_skin_001: ${ownershipRes.data.data.ownership.phoenix_skin_001}`);

        // 12. Test dev-login endpoint (for development only)
        console.log('\n12. Testing dev-login endpoint...');
        try {
            const devLoginRes = await axios.post(`${API_BASE}/auth/dev-login`, {
                email: testCredentials.email
            });
            console.log(`✅ Dev login successful`);
            console.log(`   Token: ${devLoginRes.data.data.token.substring(0, 20)}...`);
        } catch (devError) {
            console.log(`⚠️ Dev login not available: ${devError.response?.data?.message || devError.message}`);
        }

        console.log('\n🎉 All Unreal Engine API tests completed successfully!');
        console.log('\n📋 Test Summary:');
        console.log('   Backend URL: http://localhost:3000');
        console.log(`   Test User: ${testCredentials.username}`);
        console.log(`   Test Email: ${testCredentials.email}`);
        console.log(`   Test Token: ${token.substring(0, 30)}...`);
        console.log('\n🚀 Ready for Unreal Engine integration!');

    } catch (error) {
        console.error('\n❌ Test failed:');
        if (error.response) {
            console.error(`   Status: ${error.response.status}`);
            console.error(`   Message: ${error.response.data?.message || error.response.statusText}`);
            if (error.response.data?.errors) {
                console.error(`   Errors:`, error.response.data.errors);
            }
        } else {
            console.error(`   Error: ${error.message}`);
        }
        process.exit(1);
    }
}

// Run tests
testUnrealEndpoints();