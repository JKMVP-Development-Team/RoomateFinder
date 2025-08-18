import {
  Card,
  CardContent,
  Typography,
  Box,
  Chip,
  Rating,
  Avatar,
  Divider,
} from '@mui/material';
import {
  LocationOn,
  AttachMoney,
  Pets,
  SmokingRooms,
  SmokeFree,
} from '@mui/icons-material';

function UserCard({ profileData }) {
  const {
    username,
    location,
    bio,
    budget,
    cleanliness,
    smoking,
    hasPets,
    petFriendly,
    workSchedule,
    socialLevel,
    noiseTolerance,
  } = profileData;

  const getSmokingIcon = () => {
    if (smoking === 'smoker') return <SmokingRooms />;
    if (smoking === 'non-smoker') return <SmokeFree />;
    return null;
  };

  const getSmokingLabel = () => {
    if (smoking === 'smoker') return 'Smoker';
    if (smoking === 'non-smoker') return 'Non-smoker';
    return 'No preference';
  };

  return (
    <Card sx={{ height: 'fit-content', boxShadow: 3 }}>
      <CardContent sx={{ p: 3 }}>
        {/* Header with avatar and name */}
        <Box sx={{ display: 'flex', alignItems: 'center', mb: 2 }}>
          <Avatar
            sx={{
              width: 60,
              height: 60,
              bgcolor: 'primary.main',
              fontSize: '1.5rem',
              mr: 2,
            }}
          >
            {(username || 'Y').charAt(0).toUpperCase()}
          </Avatar>
          <Box>
            <Typography variant="h6" gutterBottom>
              {username || 'Your Name'}
            </Typography>
            <Box
              sx={{
                display: 'flex',
                alignItems: 'center',
                color: 'text.secondary',
              }}
            >
              <LocationOn sx={{ fontSize: 16, mr: 0.5 }} />
              <Typography variant="body2">
                {location || 'Location not specified'}
              </Typography>
            </Box>
          </Box>
        </Box>

        <Divider sx={{ my: 2 }} />

        {/* Bio */}
        {bio && (
          <Box sx={{ mb: 2 }}>
            <Typography variant="body1" gutterBottom>
              <strong>About:</strong>
            </Typography>
            <Typography variant="body2" color="text.secondary">
              {bio}
            </Typography>
          </Box>
        )}

        {/* Budget */}
        <Box sx={{ display: 'flex', alignItems: 'center', mb: 2 }}>
          <AttachMoney sx={{ color: 'success.main', mr: 1 }} />
          <Typography variant="body1">
            <strong>Budget:</strong> ${budget[0]} - ${budget[1]}/month
          </Typography>
        </Box>

        {/* Compatibility Ratings */}
        <Box sx={{ mb: 2 }}>
          <Typography variant="body1" gutterBottom>
            <strong>Compatibility:</strong>
          </Typography>
          <Box sx={{ display: 'flex', flexDirection: 'column', gap: 1 }}>
            <Box
              sx={{
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
              }}
            >
              <Typography variant="body2">Cleanliness:</Typography>
              <Box sx={{ display: 'flex', alignItems: 'center' }}>
                <Rating value={cleanliness} readOnly size="small" />
                <Typography variant="caption" sx={{ ml: 1 }}>
                  ({cleanliness}/5)
                </Typography>
              </Box>
            </Box>
            <Box
              sx={{
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
              }}
            >
              <Typography variant="body2">Social Level:</Typography>
              <Box sx={{ display: 'flex', alignItems: 'center' }}>
                <Rating value={socialLevel} readOnly size="small" />
                <Typography variant="caption" sx={{ ml: 1 }}>
                  ({socialLevel}/5)
                </Typography>
              </Box>
            </Box>
            <Box
              sx={{
                display: 'flex',
                alignItems: 'center',
                justifyContent: 'space-between',
              }}
            >
              <Typography variant="body2">Noise Tolerance:</Typography>
              <Box sx={{ display: 'flex', alignItems: 'center' }}>
                <Rating value={noiseTolerance} readOnly size="small" />
                <Typography variant="caption" sx={{ ml: 1 }}>
                  ({noiseTolerance}/5)
                </Typography>
              </Box>
            </Box>
          </Box>
        </Box>

        {/* Lifestyle chips */}
        <Box sx={{ mb: 2 }}>
          <Typography variant="body1" gutterBottom>
            <strong>Lifestyle:</strong>
          </Typography>
          <Box sx={{ display: 'flex', flexWrap: 'wrap', gap: 1 }}>
            <Chip
              icon={getSmokingIcon()}
              label={getSmokingLabel()}
              size="small"
              color={
                smoking === 'smoker'
                  ? 'warning'
                  : smoking === 'non-smoker'
                    ? 'success'
                    : 'default'
              }
            />
            <Chip
              label={
                workSchedule === '9-5'
                  ? 'Day shift'
                  : workSchedule === 'night'
                    ? 'Night shift'
                    : 'Flexible'
              }
              size="small"
              color="info"
            />
            {hasPets && (
              <Chip
                icon={<Pets />}
                label="Has pets"
                size="small"
                color="info"
              />
            )}
            {petFriendly && (
              <Chip
                icon={<Pets />}
                label="Pet-friendly"
                size="small"
                color="secondary"
              />
            )}
          </Box>
        </Box>
      </CardContent>
    </Card>
  );
}

export default UserCard;
